#include "../../src/hardware/display.h"
#include "../../src/hardware/GC9A01.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// Mock display buffer
static uint8_t display_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT * 2];  // RGB565 format
static bool display_initialized = true;
static struct GC9A01_frame last_frame;

void GC9A01_set_frame(struct GC9A01_frame frame) {
    last_frame = frame;
}

bool display_ready(void) {
    return display_initialized;
}

bool display_write_data(const uint8_t *data, uint32_t len) {
    if (!data || len > sizeof(display_buffer)) {
        return false;
    }
    memcpy(display_buffer, data, len);
    return true;
}

bool display_end_write(void) {
    return true;
}

// Mock control functions
void mock_display_set_ready(bool ready) {
    display_initialized = ready;
}

// Test helper functions
const uint8_t* mock_display_get_buffer(void) {
    return display_buffer;
}

void mock_display_clear_buffer(void) {
    memset(display_buffer, 0, sizeof(display_buffer));
}

const struct GC9A01_frame* mock_display_get_last_frame(void) {
    return &last_frame;
} 