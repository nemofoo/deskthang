#include "mock_display.h"
#include <string.h>

// Mock state
static struct {
    struct GC9A01_frame last_frame;
    uint8_t last_data[240*240*2];  // Max size for full screen RGB565
    size_t last_data_len;
    bool frame_set;
    bool data_written;
} mock_state;

void mock_display_reset(void) {
    memset(&mock_state, 0, sizeof(mock_state));
}

void GC9A01_set_frame(struct GC9A01_frame frame) {
    mock_state.last_frame = frame;
    mock_state.frame_set = true;
}

void GC9A01_write_data(const uint8_t *data, size_t len) {
    if (len <= sizeof(mock_state.last_data)) {
        memcpy(mock_state.last_data, data, len);
        mock_state.last_data_len = len;
        mock_state.data_written = true;
    }
}

bool mock_display_verify_frame(struct GC9A01_frame expected_frame) {
    if (!mock_state.frame_set) {
        return false;
    }
    
    return memcmp(&mock_state.last_frame, &expected_frame, sizeof(struct GC9A01_frame)) == 0;
}

bool mock_display_verify_data(const uint8_t *expected_data, size_t len) {
    if (!mock_state.data_written || mock_state.last_data_len != len) {
        return false;
    }
    
    return memcmp(mock_state.last_data, expected_data, len) == 0;
} 