#ifndef MOCK_PROTOCOL_H
#define MOCK_PROTOCOL_H

#include <stdint.h>

// Mock control functions
void mock_protocol_set_backoff_value(uint32_t value);
uint32_t mock_protocol_get_backoff_calls(void);

#endif // MOCK_PROTOCOL_H 