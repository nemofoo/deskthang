#include "protocol.h"
#include <string.h>
#include <stdlib.h>
#include "../state/state.h"
#include "../system/time.h"
#include <stdio.h>
#include "../protocol/command.h"
#include "../hardware/display.h"

// Error codes
#define ERROR_INVALID_VERSION 1
#define ERROR_HARDWARE_FAILURE 2

// Global protocol configuration
static ProtocolConfig g_protocol_config;

// Global error context
static ErrorDetails g_error_context;

// Add these function declarations
static bool handle_sync_packet(const Packet *packet);
static bool handle_command_packet(const Packet *packet);
static bool handle_data_packet(const Packet *packet);
static bool handle_sync_ack_packet(const Packet *packet);

// Add at the top with other static variables
static bool protocol_initialized = false;
static bool has_valid_sync = false;
static bool has_valid_command = false;
static uint8_t current_protocol_version = 0;
static CommandContext command_context = {0};

// Initialize protocol
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
    g_protocol_config.limits.max_packet_size = MAX_PACKET_SIZE;
    g_protocol_config.limits.chunk_size = CHUNK_SIZE;
    g_protocol_config.limits.header_size = HEADER_SIZE;
    
    // Reset state tracking
    g_protocol_config.last_checksum = 0;
    g_protocol_config.packets_processed = 0;
    g_protocol_config.errors_seen = 0;
    
    // Clear error context
    memset(&g_error_context, 0, sizeof(ErrorDetails));
    
    protocol_initialized = true;
    has_valid_sync = false;
    has_valid_command = false;
    current_protocol_version = 0;
    memset(&command_context, 0, sizeof(command_context));
    
    return true;
}

// Reset protocol state
void protocol_reset(void) {
    g_protocol_config.sequence = 0;
    g_protocol_config.last_checksum = 0;
    g_protocol_config.packets_processed = 0;
    g_protocol_config.errors_seen = 0;
    memset(&g_error_context, 0, sizeof(ErrorDetails));
}

// Get protocol configuration
ProtocolConfig *protocol_get_config(void) {
    return &g_protocol_config;
}

// Deinitialize protocol
void protocol_deinit(void) {
    // Reset protocol state
    protocol_reset();
    
    // Clear error context
    protocol_clear_error();
    
    protocol_initialized = false;
    has_valid_sync = false;
    has_valid_command = false;
    current_protocol_version = 0;
    memset(&command_context, 0, sizeof(command_context));
}

// Error handling
void protocol_set_error(ErrorType type, const char *message) {
    // Validate error type is appropriate for protocol module
    if (type != ERROR_TYPE_PROTOCOL && type != ERROR_TYPE_NONE) {
        // If invalid type provided, force it to protocol type
        type = ERROR_TYPE_PROTOCOL;
    }
    
    // Validate error code is in range for the type
    uint32_t error_code = g_protocol_config.errors_seen + ERROR_CODE_PROTOCOL_START;
    if (!error_code_in_range(type, error_code)) {
        // If code would be out of range, wrap back to start of range
        g_protocol_config.errors_seen = 0;
        error_code = ERROR_CODE_PROTOCOL_START;
    }
    
    g_error_context.type = type;
    g_error_context.source_state = state_machine_get_current();
    g_error_context.timestamp = deskthang_time_get_ms();
    g_error_context.code = error_code;
    g_protocol_config.errors_seen++;
    
    if (message) {
        strncpy(g_error_context.message, message, sizeof(g_error_context.message) - 1);
        g_error_context.message[sizeof(g_error_context.message) - 1] = '\0';
    } else {
        g_error_context.message[0] = '\0';
    }
    
    // Determine if error is recoverable based on error code
    if (error_code >= ERROR_CODE_PROTOCOL_START && error_code <= ERROR_CODE_PROTOCOL_END) {
        // Protocol errors are generally recoverable unless explicitly marked
        g_error_context.recoverable = true;
        
        // Some specific protocol errors are not recoverable
        switch (error_code) {
            case ERROR_CODE_PROTOCOL_VERSION_MISMATCH:
            case ERROR_CODE_PROTOCOL_FATAL:
                g_error_context.recoverable = false;
                break;
            default:
                break;
        }
    } else {
        // Non-protocol errors in protocol module shouldn't happen
        g_error_context.recoverable = false;
    }
    
    // Reset retry count for new error
    g_error_context.retry_count = 0;
    g_error_context.backoff_ms = MIN_RETRY_DELAY_MS;
}

// Get current error context
ErrorDetails *protocol_get_error(void) {
    return &g_error_context;
}

// Clear error state
bool protocol_clear_error(void) {
    memset(&g_error_context, 0, sizeof(ErrorDetails));
    return true;
}

// Calculate exponential backoff delay
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

// Determine if retry should be attempted
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

// Protocol validation functions
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

bool protocol_process_packet(const Packet *packet) {
    if (!packet) {
        protocol_set_error(ERROR_TYPE_PROTOCOL, "Null packet");
        return false;
    }

    // Validate packet
    if (!packet_validate(packet)) {
        protocol_set_error(ERROR_TYPE_PROTOCOL, "Invalid packet");
        return false;
    }

    // Process based on packet type
    switch (packet->header.type) {
        case PACKET_TYPE_SYNC:
            has_valid_sync = handle_sync_packet(packet);
            if (has_valid_sync) {
                current_protocol_version = packet->payload[0];
            }
            break;
        case PACKET_TYPE_SYNC_ACK:
            return handle_sync_ack_packet(packet);
        case PACKET_TYPE_CMD:
            has_valid_command = handle_command_packet(packet);
            if (has_valid_command) {
                memcpy(&command_context, packet->payload, sizeof(CommandContext));
            }
            break;
        case PACKET_TYPE_DATA:
            return handle_data_packet(packet);
        default:
            protocol_set_error(ERROR_TYPE_PROTOCOL, "Unknown packet type");
            return false;
    }

    return true;
}

// Add implementations
static bool handle_sync_packet(const Packet *packet) {
    if (!packet || packet->header.type != PACKET_TYPE_SYNC) {
        protocol_set_error(ERROR_TYPE_PROTOCOL, "Invalid SYNC packet");
        return false;
    }

    // Validate protocol version
    uint8_t version = packet->payload[0];
    if (!protocol_validate_version(version)) {
        char context[64];
        snprintf(context, sizeof(context), "Expected v%d, got v%d", PROTOCOL_VERSION, version);
        protocol_set_error(ERROR_TYPE_PROTOCOL, "Protocol version mismatch");
        
        // Send NACK for version mismatch
        Packet nack;
        if (packet_create_nack(&nack, packet->header.sequence)) {
            packet_transmit(&nack);
        }
        return false;
    }

    // Reset protocol state for new connection
    protocol_reset();
    g_protocol_config.sequence = packet->header.sequence;

    // Create and send SYNC_ACK response
    Packet sync_ack;
    if (!packet_create_sync_ack(&sync_ack)) {
        protocol_set_error(ERROR_TYPE_PROTOCOL, "Failed to create SYNC_ACK");
        return false;
    }

    // Transmit SYNC_ACK
    if (!packet_transmit(&sync_ack)) {
        protocol_set_error(ERROR_TYPE_PROTOCOL, "Failed to transmit SYNC_ACK");
        return false;
    }

    // Update protocol state to synchronized
    g_protocol_config.version = version;
    return true;
}

static bool handle_command_packet(const Packet *packet) {
    // TODO: Implement command packet handling
    return true;
}

static bool handle_data_packet(const Packet *packet) {
    // TODO: Implement data packet handling
    return true;
}

static bool handle_sync_ack_packet(const Packet *packet) {
    if (!packet || packet->header.type != PACKET_TYPE_SYNC_ACK) {
        protocol_set_error(ERROR_TYPE_PROTOCOL, "Invalid SYNC_ACK packet");
        return false;
    }

    // Validate protocol version in response
    uint8_t version = packet->payload[0];
    if (!protocol_validate_version(version)) {
        protocol_set_error(ERROR_TYPE_PROTOCOL, "Protocol version mismatch in SYNC_ACK");
        return false;
    }

    // Validate sequence number matches our SYNC
    if (!protocol_validate_sequence(packet->header.sequence)) {
        protocol_set_error(ERROR_TYPE_PROTOCOL, "Invalid sequence in SYNC_ACK");
        return false;
    }

    // Update protocol state
    g_protocol_config.version = version;
    g_protocol_config.sequence = packet->header.sequence;

    return true;
}

bool protocol_timing_valid(void) {
    // TODO: Implement proper validation
    return true;
}

bool protocol_is_synchronized(void) {
    // Protocol is synchronized if:
    // 1. It is initialized
    // 2. Version matches
    // 3. No active errors
    return protocol_is_initialized() && 
           g_protocol_config.version == PROTOCOL_VERSION &&
           g_error_context.type == ERROR_TYPE_NONE;
}

bool protocol_is_initialized(void) {
    return protocol_initialized;
}

bool protocol_has_valid_sync(void) {
    return has_valid_sync;
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
