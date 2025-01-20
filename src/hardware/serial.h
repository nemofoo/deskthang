#ifndef DESKTHANG_SERIAL_H
#define DESKTHANG_SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../common/deskthang_constants.h"

// Serial configuration
#define SERIAL_WRITE_TIMEOUT_MS 100

// Error codes
#define ERROR_SERIAL_OVERFLOW 1001
#define ERROR_SERIAL_TIMEOUT  1002

// Statistics structure
typedef struct {
    uint32_t overflow_count;      // Number of overflow events
    uint32_t last_overflow_time;  // Timestamp of last overflow
    bool in_overflow;             // Currently in overflow state
} SerialStats;

// Core serial functions
bool serial_init(void);
void serial_deinit(void);
bool serial_write(const uint8_t *data, size_t len);
bool serial_write_chunk(const uint8_t *data, size_t len);
bool serial_read(uint8_t *data, size_t len);
bool serial_write_debug(const char *module, const char *message);
bool serial_write_chunked(const uint8_t *data, size_t len);
void serial_flush(void);
bool serial_available(void);
void serial_clear(void);

// Statistics and monitoring
bool serial_get_stats(SerialStats *stats);

#endif // DESKTHANG_SERIAL_H
