#include "../../src/system/time.h"

static uint32_t mock_current_time_ms = 0;
static uint32_t mock_delay_calls = 0;

uint32_t deskthang_time_get_ms(void) {
    return mock_current_time_ms;
}

void deskthang_delay_ms(uint32_t delay_ms) {
    mock_current_time_ms += delay_ms;
    mock_delay_calls++;
}

// Test helper functions
void mock_time_set(uint32_t time_ms) {
    mock_current_time_ms = time_ms;
    mock_delay_calls = 0;
}

void mock_time_advance(uint32_t delta_ms) {
    mock_current_time_ms += delta_ms;
}

uint32_t mock_time_get_delay_calls(void) {
    return mock_delay_calls;
} 