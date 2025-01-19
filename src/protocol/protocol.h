#ifndef DESKTHANG_PROTOCOL_H
#define DESKTHANG_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "../state/state.h"

// Protocol version
#define PROTOCOL_VERSION 1

// Buffer sizes
#define MAX_PACKET_SIZE 512  // Maximum packet size in bytes
#define CHUNK_SIZE 256      // Size of data transfer chunks
#define HEADER_SIZE 8       // Size of packet header in bytes

// Timing constants
#define BASE_TIMEOUT_MS 1000    // Base timeout for operations
#define MIN_RETRY_DELAY_MS 50   // Minimum delay between retries
#define MAX_RETRY_DELAY_MS 1000 // Maximum delay between retries
#define MAX_RETRIES 8          // Maximum number of retry attempts

// Protocol configuration
typedef struct {
    uint8_t version;          // Protocol version
    uint8_t sequence;         // Current sequence number
    
    struct {
        uint32_t base_timeout_ms;      // Base timeout
        uint32_t min_retry_delay_ms;   // Min retry delay
        uint32_t max_retry_delay_ms;   // Max retry delay
        uint8_t max_retries;           // Max retry attempts
    } timing;
    
    struct {
        uint16_t max_packet_size;     // Max packet size
        uint16_t chunk_size;          // Transfer chunk size
        uint8_t header_size;          // Header size in bytes
    } limits;
    
    // State tracking
    uint32_t last_checksum;          // Last valid checksum
    uint32_t packets_processed;       // Packet counter
    uint32_t errors_seen;            // Error counter
} ProtocolConfig;

// Error types
typedef enum {
    PROTOCOL_ERROR_NONE,
    PROTOCOL_ERROR_INVALID_VERSION,
    PROTOCOL_ERROR_INVALID_SEQUENCE,
    PROTOCOL_ERROR_INVALID_LENGTH,
    PROTOCOL_ERROR_INVALID_CHECKSUM,
    PROTOCOL_ERROR_INVALID_COMMAND,
    PROTOCOL_ERROR_BUFFER_OVERFLOW,
    PROTOCOL_ERROR_TIMEOUT,
    PROTOCOL_ERROR_STATE_MISMATCH,
    PROTOCOL_ERROR_HARDWARE_FAILURE
} ProtocolErrorType;

// Error context
typedef struct {
    // Error details
    ProtocolErrorType type;      // Error classification
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
void protocol_set_error(ProtocolErrorType type, const char *message);
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
