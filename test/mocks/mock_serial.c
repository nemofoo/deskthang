#include "mock_serial.h"
#include "../../src/hardware/serial.h"
#include <string.h>

#define MAX_BUFFER_SIZE 1024

static struct {
    uint8_t read_buffer[MAX_BUFFER_SIZE];
    uint16_t read_buffer_size;
    uint16_t read_position;
    
    uint8_t write_buffer[MAX_BUFFER_SIZE];
    uint16_t write_buffer_size;
    
    uint32_t write_count;
    uint32_t read_count;
    uint32_t flush_count;
} mock_serial_state = {0};

bool serial_init(void) {
    mock_serial_reset();
    return true;
}

bool serial_write(const uint8_t* data, size_t length) {
    if (!data || length == 0) return false;
    if (mock_serial_state.write_buffer_size + length > MAX_BUFFER_SIZE) return false;
    
    memcpy(mock_serial_state.write_buffer + mock_serial_state.write_buffer_size, 
           data, length);
    mock_serial_state.write_buffer_size += length;
    mock_serial_state.write_count++;
    return true;
}

bool serial_read(uint8_t* data, size_t length) {
    if (!data || length == 0) return false;
    if (mock_serial_state.read_position >= mock_serial_state.read_buffer_size) return false;
    
    uint16_t available = mock_serial_state.read_buffer_size - mock_serial_state.read_position;
    uint16_t to_read = (length < available) ? length : available;
    
    memcpy(data, mock_serial_state.read_buffer + mock_serial_state.read_position, to_read);
    mock_serial_state.read_position += to_read;
    mock_serial_state.read_count++;
    return true;
}

void serial_flush(void) {
    mock_serial_state.flush_count++;
}

// Mock control functions
void mock_serial_reset(void) {
    memset(&mock_serial_state, 0, sizeof(mock_serial_state));
}

void mock_serial_set_read_data(const uint8_t* data, uint16_t length) {
    if (!data || length > MAX_BUFFER_SIZE) return;
    
    memcpy(mock_serial_state.read_buffer, data, length);
    mock_serial_state.read_buffer_size = length;
    mock_serial_state.read_position = 0;
}

void mock_serial_get_written_data(uint8_t* buffer, uint16_t* length) {
    if (!buffer || !length) return;
    
    memcpy(buffer, mock_serial_state.write_buffer, mock_serial_state.write_buffer_size);
    *length = mock_serial_state.write_buffer_size;
}

// Statistics
uint32_t mock_serial_get_write_count(void) {
    return mock_serial_state.write_count;
}

uint32_t mock_serial_get_read_count(void) {
    return mock_serial_state.read_count;
}

uint32_t mock_serial_get_flush_count(void) {
    return mock_serial_state.flush_count;
} 