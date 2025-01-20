#ifndef MOCK_SERIAL_H
#define MOCK_SERIAL_H

#include <stdint.h>
#include <stdbool.h>

// Mock control functions
void mock_serial_reset(void);
void mock_serial_set_read_data(const uint8_t* data, uint16_t length);
void mock_serial_get_written_data(uint8_t* buffer, uint16_t* length);

// Statistics
uint32_t mock_serial_get_write_count(void);
uint32_t mock_serial_get_read_count(void);
uint32_t mock_serial_get_flush_count(void);

#endif // MOCK_SERIAL_H 