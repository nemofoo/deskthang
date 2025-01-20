#include "mock_protocol.h"
#include "../../src/protocol/protocol.h"

static struct {
    uint32_t backoff_value;
    uint32_t backoff_calls;
} mock_protocol_state = {0};

uint32_t protocol_calculate_backoff(uint8_t retry_count) {
    mock_protocol_state.backoff_calls++;
    return mock_protocol_state.backoff_value;
}

void mock_protocol_set_backoff_value(uint32_t value) {
    mock_protocol_state.backoff_value = value;
}

uint32_t mock_protocol_get_backoff_calls(void) {
    return mock_protocol_state.backoff_calls;
} 