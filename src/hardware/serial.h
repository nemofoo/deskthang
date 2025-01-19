#ifndef DESKTHANG_SERIAL_H
#define DESKTHANG_SERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include "../protocol/protocol.h"
#include "../error/error.h"

// Initialize USB serial with specified baud rate
bool serial_init(uint32_t baud_rate);

// Read exactly size bytes into buffer with timeout
// Returns true if all bytes were read, false on timeout/error
bool serial_read_exact(uint8_t *buffer, uint16_t size, uint32_t timeout_ms);

// Read up to size bytes into buffer with timeout
// Returns number of bytes read, or -1 on error
int16_t serial_read(uint8_t *buffer, uint16_t size, uint32_t timeout_ms);

// Write exactly size bytes from buffer with flush
// Returns true if all bytes were written, false on error
bool serial_write_exact(const uint8_t *buffer, uint16_t size);

// Write up to size bytes from buffer
// Returns number of bytes written, or -1 on error
int16_t serial_write(const uint8_t *buffer, uint16_t size);

// Flush the transmit buffer
void serial_flush(void);

// Check if serial data is available
bool serial_available(void);

// Clear receive buffer
void serial_clear(void);

// Close serial connection
void serial_deinit(void);

#endif // DESKTHANG_SERIAL_H
