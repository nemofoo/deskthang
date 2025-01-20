#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "../error/error.h"
#include "../common/deskthang_constants.h"  // Updated include
#include "packet.h"  // Then get packet definitions

// Protocol configuration structure
typedef struct {
    uint8_t version;          // Must match PROTOCOL_VERSION
    uint8_t sequence;         // Current sequence number
    
    // Timing configuration
    struct {
        uint32_t base_timeout_ms;      // BASE_TIMEOUT_MS
        uint32_t min_retry_delay_ms;   // MIN_RETRY_DELAY_MS
        uint32_t max_retry_delay_ms;   // MAX_RETRY_DELAY_MS
        uint8_t max_retries;           // MAX_RETRIES
    } timing;
    
    // Buffer configuration
    struct {
        uint16_t max_packet_size;     // MAX_PACKET_SIZE
        uint16_t chunk_size;          // CHUNK_SIZE
        uint8_t header_size;          // HEADER_SIZE
    } limits;
    
    // State tracking
    uint32_t last_checksum;          // Last valid checksum
    uint32_t packets_processed;       // Packet counter
    uint32_t errors_seen;            // Error counter
} ProtocolConfig;

// Core protocol functions
bool protocol_init(const ProtocolConfig *config);
void protocol_deinit(void);
void protocol_reset(void);
ProtocolConfig *protocol_get_config(void);

// Error handling
void protocol_set_error(ErrorType type, const char *message);
ErrorDetails *protocol_get_error(void);
bool protocol_clear_error(void);

// Retry and backoff
uint32_t protocol_calculate_backoff(uint8_t retry_count);
bool protocol_should_retry(ErrorDetails *ctx);

// Validation functions
bool protocol_validate_version(uint8_t version);
bool protocol_validate_sequence(uint8_t sequence);
bool protocol_validate_length(uint16_t length);
bool protocol_validate_checksum(uint32_t checksum, const uint8_t *data, uint16_t length);

bool protocol_process_packet(const Packet *packet);

bool protocol_timing_valid(void);

// Add these declarations
bool transfer_buffer_available(void);
bool transfer_sequence_valid(void);
bool transfer_checksum_valid(void);

// Protocol state functions
bool protocol_is_initialized(void);
bool protocol_is_synchronized(void);

#endif // PROTOCOL_H
