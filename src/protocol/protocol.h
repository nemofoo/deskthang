#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "packet.h"
#include "../state/state.h"
#include "../common/deskthang_constants.h"
#include "../error/error.h"

// Protocol version
#define PROTOCOL_VERSION 1

// Timing constants
#define BASE_TIMEOUT_MS 1000

// Protocol configuration
typedef struct {
    uint8_t version;
    uint8_t sequence;
    struct {
        uint32_t base_timeout_ms;
        uint32_t min_retry_delay_ms;
        uint32_t max_retry_delay_ms;
        uint8_t max_retries;
    } timing;
    struct {
        uint16_t max_packet_size;
        uint16_t chunk_size;
        uint16_t header_size;
    } limits;
} ProtocolConfig;

// Protocol initialization and cleanup
bool protocol_init(const ProtocolConfig *config);
void protocol_deinit(void);
void protocol_reset(void);

// Protocol state
bool protocol_is_initialized(void);
bool protocol_is_synchronized(void);
bool protocol_has_valid_sync(void);
bool protocol_version_valid(void);
bool protocol_has_valid_command(void);
bool protocol_command_params_valid(void);
bool protocol_command_resources_available(void);
ProtocolConfig *protocol_get_config(void);

// Packet processing
bool protocol_process_packet(const Packet *packet);
bool protocol_validate_packet(const Packet *packet);
bool protocol_validate_version(uint8_t version);
bool protocol_validate_sequence(uint8_t sequence);
bool protocol_validate_length(uint16_t length);
bool protocol_validate_checksum(uint32_t checksum, const uint8_t *data, uint16_t length);

// Error handling
void protocol_set_error(const char *module, const char *message);
ErrorDetails *protocol_get_error(void);
bool protocol_clear_error(void);

// Retry handling
uint32_t protocol_calculate_backoff(uint8_t retry_count);
bool protocol_should_retry(ErrorDetails *ctx);

#endif // PROTOCOL_H
