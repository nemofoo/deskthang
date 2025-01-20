#ifndef MOCK_TIME_H
#define MOCK_TIME_H

#include <stdint.h>

// Test helper functions
void mock_time_set(uint32_t time_ms);
void mock_time_advance(uint32_t delta_ms);
uint32_t mock_time_get_delay_calls(void);

#endif // MOCK_TIME_H 