#ifndef __DESKTHANG_SERIAL_H
#define __DESKTHANG_SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "protocol/protocol.h"  // For MAX_PACKET_SIZE and CHUNK_SIZE

// Debug packet structure
typedef struct {
    uint32_t timestamp;
    char module[32];
    char message[256];
} __attribute__((packed)) DebugPacket;

// Core functions
bool serial_init(void);
void serial_deinit(void);
bool serial_write(const uint8_t *data, size_t len);
bool serial_read(uint8_t *data, size_t len);

// Buffer management
void serial_flush(void);
bool serial_available(void);
void serial_clear(void);

// Debug support
bool serial_write_debug(const char *module, const char *message);
bool serial_write_chunked(const uint8_t *data, size_t len);

#endif // __DESKTHANG_SERIAL_H
