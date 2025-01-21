#include "protocol.h"
#include "transfer.h"
#include <string.h>
#include <stdlib.h>
#include "../state/state.h"
#include "../system/time.h"
#include <stdio.h>
#include "../protocol/command.h"
#include "../hardware/display.h"
#include "../error/logging.h"

// Error codes
#define ERROR_INVALID_VERSION 1
#define ERROR_HARDWARE_FAILURE 2

// Global protocol configuration
static ProtocolConfig g_protocol_config;

// Global error context
static ErrorDetails g_error_context;

// Protocol state
static bool protocol_initialized = false;
static bool has_valid_sync = false;
static bool has_valid_command = false;
static uint8_t current_protocol_version = 0;
static CommandContext command_context = {0};

// Static function declarations
static bool handle_sync_packet(const Packet *packet);
static bool handle_command_packet(const Packet *packet);
static bool handle_data_packet(const Packet *packet);
static bool handle_error_packet(const Packet *packet);

bool protocol_init(const ProtocolConfig *config) {
    if (!config) {
        return false;
    }
    
    // Copy configuration
    memcpy(&g_protocol_config, config, sizeof(ProtocolConfig));
    
    // Initialize protocol configuration
    g_protocol_config.version = PROTOCOL_VERSION;
    g_protocol_config.sequence = 0;
    
    // Set timing configuration
    g_protocol_config.timing.base_timeout_ms = BASE_TIMEOUT_MS;
    g_protocol_config.timing.min_retry_delay_ms = MIN_RETRY_DELAY_MS;
    g_protocol_config.timing.max_retry_delay_ms = MAX_RETRY_DELAY_MS;
    g_protocol_config.timing.max_retries = MAX_RETRIES;
    
    // Set buffer configuration
    g_protocol_config.limits.max_packet_size = MAX_PAYLOAD_SIZE;
    g_protocol_config.limits.chunk_size = 256;  // Reasonable default
    g_protocol_config.limits.header_size = sizeof(PacketHeader);
    
    protocol_initialized = true;
    has_valid_sync = false;
    has_valid_command = false;
    current_protocol_version = 0;
    memset(&command_context, 0, sizeof(command_context));
    
    return true;
}

void protocol_reset(void) {
    g_protocol_config.sequence = 0;
    memset(&g_error_context, 0, sizeof(ErrorDetails));
}

ProtocolConfig *protocol_get_config(void) {
    return &g_protocol_config;
}

void protocol_deinit(void) {
    protocol_reset();
    protocol_initialized = false;
    has_valid_sync = false;
    has_valid_command = false;
    current_protocol_version = 0;
    memset(&command_context, 0, sizeof(command_context));
}

void protocol_set_error(const char *module, const char *message) {
    // Create and send error packet
    Packet error;
    if (packet_create_error(&error, module, message)) {
        packet_transmit(&error);
        packet_free(&error);
    }
}

ErrorDetails *protocol_get_error(void) {
    return &g_error_context;
}

bool protocol_clear_error(void) {
    memset(&g_error_context, 0, sizeof(ErrorDetails));
    return true;
}

uint32_t protocol_calculate_backoff(uint8_t retry_count) {
    uint32_t delay = MIN_RETRY_DELAY_MS;
    
    // Exponential backoff with max limit
    for (uint8_t i = 0; i < retry_count && delay < MAX_RETRY_DELAY_MS; i++) {
        delay *= 2;
    }
    
    // Add small random jitter (0-50ms)
    delay += (rand() % 50);
    
    // Ensure we don't exceed max delay
    return (delay > MAX_RETRY_DELAY_MS) ? MAX_RETRY_DELAY_MS : delay;
}

bool protocol_should_retry(ErrorDetails *ctx) {
    if (!ctx->recoverable) {
        return false;
    }
    
    if (ctx->retry_count >= MAX_RETRIES) {
        return false;
    }
    
    // Calculate new backoff delay
    ctx->backoff_ms = protocol_calculate_backoff(ctx->retry_count);
    ctx->retry_count++;
    
    return true;
}

bool protocol_validate_version(uint8_t version) {
    return version == PROTOCOL_VERSION;
}

bool protocol_validate_sequence(uint8_t sequence) {
    // Sequence numbers should increment by 1
    uint8_t expected = g_protocol_config.sequence + 1;
    return sequence == expected;
}

bool protocol_validate_length(uint16_t length) {
    return length <= MAX_PACKET_SIZE;
}

bool protocol_validate_checksum(uint32_t checksum, const uint8_t *data, uint16_t length) {
    // TODO: Implement CRC32 calculation
    return true;
}

bool protocol_validate_packet(const Packet *packet) {
    if (!packet) {
        return false;
    }
    
    // Basic validation is handled by packet_validate()
    if (!packet_validate(packet)) {
        return false;
    }
    
    // Protocol-specific validation
    if (packet->header.type == PACKET_TYPE_SYNC) {
        // Sync packets must have exactly 1 byte payload (version)
        if (packet->header.length != 1) {
            return false;
        }
        // Version must match
        if (packet->payload[0] != PROTOCOL_VERSION) {
            return false;
        }
    }
    
    // Sequence validation (except for SYNC which can reset sequence)
    if (packet->header.type != PACKET_TYPE_SYNC) {
        uint8_t expected = g_protocol_config.sequence + 1;
        if (packet->header.sequence != expected) {
            return false;
        }
    }
    
    return true;
}

bool protocol_process_packet(const Packet *packet) {
    if (!protocol_validate_packet(packet)) {
        packet_create_nack(packet, packet->header.sequence, "Invalid packet");
        return false;
    }
    
    bool result = false;
    switch (packet->header.type) {
        case PACKET_TYPE_SYNC:
            result = handle_sync_packet(packet);
            break;
            
        case PACKET_TYPE_COMMAND:
            if (!has_valid_sync) {
                packet_create_nack(packet, packet->header.sequence, "Not synchronized");
                return false;
            }
            result = handle_command_packet(packet);
            break;
            
        case PACKET_TYPE_DATA:
            if (!has_valid_sync) {
                packet_create_nack(packet, packet->header.sequence, "Not synchronized");
                return false;
            }
            result = handle_data_packet(packet);
            break;
            
        case PACKET_TYPE_ERROR:
            result = handle_error_packet(packet);
            break;
            
        default:
            packet_create_nack(packet, packet->header.sequence, "Unknown packet type");
            return false;
    }
    
    if (result) {
        g_protocol_config.sequence = packet->header.sequence;
    }
    
    return result;
}

static bool handle_sync_packet(const Packet *packet) {
    // Version check already done in validate_packet
    
    // Reset protocol state
    protocol_reset();
    has_valid_sync = true;
    
    // Send ACK
    Packet response;
    if (!packet_create_ack(&response, packet->header.sequence)) {
        return false;
    }
    
    return packet_transmit(&response);
}

static bool handle_command_packet(const Packet *packet) {
    // Command processing would go here
    // For now, just ACK
    Packet response;
    if (!packet_create_ack(&response, packet->header.sequence)) {
        return false;
    }
    
    return packet_transmit(&response);
}

static bool handle_data_packet(const Packet *packet) {
    // Data processing would go here
    // For now, just ACK
    Packet response;
    if (!packet_create_ack(&response, packet->header.sequence)) {
        return false;
    }
    
    return packet_transmit(&response);
}

static bool handle_error_packet(const Packet *packet) {
    // Log the error
    if (packet->payload && packet->header.length > 0) {
        logging_write("Protocol", (const char*)packet->payload);
    }
    
    // Don't respond to error packets
    return true;
}

bool protocol_timing_valid(void) {
    // TODO: Implement proper validation
    return true;
}

bool protocol_is_initialized(void) {
    return protocol_initialized;
}

bool protocol_is_synchronized(void) {
    return protocol_initialized && has_valid_sync;
}

bool protocol_version_valid(void) {
    return current_protocol_version == PROTOCOL_VERSION;
}

bool protocol_has_valid_command(void) {
    return has_valid_command;
}

bool protocol_command_params_valid(void) {
    if (!has_valid_command) {
        return false;
    }
    
    // Check command parameters based on command type
    switch (command_context.type) {
        case CMD_PATTERN_CHECKER:
        case CMD_PATTERN_STRIPE:
        case CMD_PATTERN_GRADIENT:
            return true;  // These commands have no parameters
            
        case CMD_IMAGE_DATA:
            return command_context.data_size > 0 && 
                   command_context.data_size <= MAX_PACKET_SIZE;
            
        default:
            return false;
    }
}

bool protocol_command_resources_available(void) {
    if (!has_valid_command) {
        return false;
    }
    
    // Check resource requirements based on command type
    switch (command_context.type) {
        case CMD_PATTERN_CHECKER:
        case CMD_PATTERN_STRIPE:
        case CMD_PATTERN_GRADIENT:
            return display_buffer_available();
            
        case CMD_IMAGE_DATA:
            return transfer_buffer_available() && 
                   display_buffer_available();
            
        default:
            return false;
    }
}
