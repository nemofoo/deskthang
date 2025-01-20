#include "protocol.h"
#include <string.h>
#include <stdlib.h>
#include "../state/state.h"
#include "../system/time.h"

// Error codes
#define ERROR_INVALID_VERSION 1000
#define ERROR_HARDWARE_FAILURE 1001

// Global protocol configuration
static ProtocolConfig g_protocol_config;

// Global error context
static ErrorDetails g_error_context;

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
}

// Error handling
void protocol_set_error(ErrorType type, const char *message) {
    g_error_context.type = type;
    g_error_context.source_state = state_machine_get_current();
    g_error_context.timestamp = get_system_time();
    g_error_context.code = g_protocol_config.errors_seen++;
    
    if (message) {
        strncpy(g_error_context.message, message, sizeof(g_error_context.message) - 1);
        g_error_context.message[sizeof(g_error_context.message) - 1] = '\0';
    } else {
        g_error_context.message[0] = '\0';
    }
    
    // Determine if error is recoverable
    switch (type) {
        case ERROR_INVALID_VERSION:
        case ERROR_HARDWARE_FAILURE:
            g_error_context.recoverable = false;
            break;
            
        default:
            g_error_context.recoverable = true;
            break;
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
