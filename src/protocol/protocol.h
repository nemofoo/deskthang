#ifndef DESKTHANG_PROTOCOL_H
#define DESKTHANG_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "../state/state.h"

// Protocol version and constants
#define PROTOCOL_VERSION 1
#define MAX_PACKET_SIZE 512
#define CHUNK_SIZE 256
#define HEADER_SIZE 8
#define BASE_TIMEOUT_MS 1000
#define MIN_RETRY_DELAY_MS 50
#define MAX_RETRY_DELAY_MS 1000
#define MAX_RETRIES 8

// Protocol configuration
typedef struct {
    // Protocol version
    uint8_t version;          // Must be 1
    uint8_t sequence;         // Current sequence number
    
    // Timing configuration
    struct {
        uint32_t base_timeout_ms;      // Base timeout (1000ms)
        uint32_t min_retry_delay_ms;   // Min retry delay (50ms)
        uint32_t max_retry_delay_ms;   // Max retry delay (1000ms)
        uint8_t max_retries;           // Max retry attempts (8)
    } timing;
    
    // Buffer configuration
    struct {
        uint16_t max_packet_size;     // Max packet size (512)
        uint16_t chunk_size;          // Transfer chunk size (256)
        uint8_t header_size;          // Header size in bytes (8)
    } limits;
    
    // State tracking
    uint32_t last_checksum;          // Last valid checksum
    uint32_t packets_processed;       // Packet counter
    uint32_t errors_seen;            // Error counter
} ProtocolConfig;

// Error types
typedef enum {
    ERROR_NONE,
    ERROR_INVALID_VERSION,
    ERROR_INVALID_SEQUENCE,
    ERROR_INVALID_LENGTH,
    ERROR_INVALID_CHECKSUM,
    ERROR_INVALID_COMMAND,
    ERROR_BUFFER_OVERFLOW,
    ERROR_TIMEOUT,
    ERROR_STATE_MISMATCH,
    ERROR_HARDWARE_FAILURE
} ErrorType;

// Error context
typedef struct {
    // Error details
    ErrorType type;              // Error classification
    SystemState source_state;    // State where error occurred
    uint32_t timestamp;         // Error timestamp
    
    // Context data
    uint32_t error_code;        // Specific error code
    char message[64];           // Error description
    
    // Recovery
    uint8_t retry_count;        // Recovery attempts
    uint32_t backoff_ms;        // Current backoff delay
    bool recoverable;           // Can be recovered from
} ErrorContext;

// Protocol initialization and management
bool protocol_init(void);
void protocol_reset(void);
ProtocolConfig *protocol_get_config(void);

// Error handling
void protocol_set_error(ErrorType type, const char *message);
ErrorContext *protocol_get_error(void);
bool protocol_clear_error(void);

// Timing functions
uint32_t protocol_calculate_backoff(uint8_t retry_count);
bool protocol_should_retry(ErrorContext *ctx);

// Validation
bool protocol_validate_version(uint8_t version);
bool protocol_validate_sequence(uint8_t sequence);
bool protocol_validate_length(uint16_t length);
bool protocol_validate_checksum(uint32_t checksum, const uint8_t *data, uint16_t length);

#endif // DESKTHANG_PROTOCOL_H
