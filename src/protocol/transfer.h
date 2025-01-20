#ifndef DESKTHANG_TRANSFER_H
#define DESKTHANG_TRANSFER_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol.h"
#include "packet.h"

// Transfer modes
typedef enum {
    TRANSFER_MODE_NONE,
    TRANSFER_MODE_IMAGE,      // RGB565 image transfer
} TransferMode;

// Transfer state
typedef enum {
    TRANSFER_STATE_IDLE,
    TRANSFER_STATE_STARTING,
    TRANSFER_STATE_IN_PROGRESS,
    TRANSFER_STATE_COMPLETING,
    TRANSFER_STATE_ERROR
} TransferState;

// Transfer context
typedef struct {
    // Transfer configuration
    TransferMode mode;          // Current transfer mode
    TransferState state;        // Current transfer state
    uint32_t start_time;        // Transfer start timestamp
    
    // Progress tracking
    uint32_t bytes_received;    // Bytes received so far
    uint32_t bytes_expected;    // Total expected bytes
    uint32_t chunks_received;   // Number of chunks received
    uint32_t chunks_expected;   // Expected number of chunks
    
    // Buffer management
    uint8_t *buffer;           // Transfer buffer
    uint32_t buffer_size;      // Buffer size
    uint32_t buffer_offset;    // Current write position
    
    // Validation
    uint32_t last_sequence;    // Last sequence number
    uint32_t last_checksum;    // Last valid checksum
    bool checksum_valid;       // Last chunk checksum valid
    
    // Error tracking
    uint32_t error_count;      // Number of errors
    uint32_t retry_count;      // Number of retries
} TransferContext;

// Core transfer functions
bool transfer_init(void);
void transfer_reset(void);
bool transfer_is_initialized(void);
TransferContext *transfer_get_context(void);

// Transfer control
bool transfer_start(TransferMode mode, uint32_t total_size);
bool transfer_process_chunk(const Packet *packet);
bool transfer_complete(void);
bool transfer_abort(void);

// Buffer management
bool transfer_allocate_buffer(uint32_t size);
void transfer_free_buffer(void);
uint8_t *transfer_get_buffer(void);
uint32_t transfer_get_buffer_size(void);

// Progress tracking
float transfer_get_progress(void);
uint32_t transfer_get_remaining_bytes(void);
uint32_t transfer_get_elapsed_time(void);
uint32_t transfer_get_estimated_time_remaining(void);

// Validation
bool transfer_validate_chunk(const Packet *packet);
bool transfer_validate_sequence(uint8_t sequence);
bool transfer_validate_checksum(const uint8_t *data, uint16_t length, uint32_t checksum);

// Error handling
bool transfer_handle_error(ErrorType error);
bool transfer_should_retry(void);
uint32_t transfer_get_retry_delay(void);

// Status reporting
typedef struct {
    bool active;                // Transfer is in progress
    float progress;            // Progress percentage
    uint32_t speed_bps;        // Transfer speed (bytes/sec)
    uint32_t errors;           // Error count
    char message[64];          // Status message
} TransferStatus;

TransferStatus *transfer_get_status(void);
void transfer_update_status(const char *message);

// Debug support
void transfer_print_status(void);
const char *transfer_mode_to_string(TransferMode mode);
const char *transfer_state_to_string(TransferState state);

#endif // DESKTHANG_TRANSFER_H
