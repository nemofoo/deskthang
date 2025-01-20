#ifndef MOCK_DISPLAY_H
#define MOCK_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "../../src/hardware/GC9A01.h"

// Mock functions
void GC9A01_set_frame(struct GC9A01_frame frame);
void GC9A01_write_data(const uint8_t *data, size_t len);

// Test helper functions
void mock_display_reset(void);
bool mock_display_verify_frame(struct GC9A01_frame expected_frame);
bool mock_display_verify_data(const uint8_t *expected_data, size_t len);

#endif // MOCK_DISPLAY_H 