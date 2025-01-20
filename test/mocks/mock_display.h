#ifndef MOCK_DISPLAY_H
#define MOCK_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>
#include "../../src/hardware/GC9A01.h"

// Mock control functions
void mock_display_set_ready(bool ready);

// Test helper functions
const uint8_t* mock_display_get_buffer(void);
void mock_display_clear_buffer(void);
const struct GC9A01_frame* mock_display_get_last_frame(void);

#endif // MOCK_DISPLAY_H 