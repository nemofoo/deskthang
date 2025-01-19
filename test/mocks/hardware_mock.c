#include "hardware_mock.h"

HardwareConfig mock_hw_config = {0};
bool mock_hw_initialized = false;
bool mock_display_ready = false;

void mock_hardware_reset(void) {
    memset(&mock_hw_config, 0, sizeof(HardwareConfig));
    mock_hw_initialized = false;
    mock_display_ready = false;
}

void mock_hardware_set_initialized(bool initialized) {
    mock_hw_initialized = initialized;
    mock_hw_config.initialized = initialized;
}

void mock_hardware_set_display_ready(bool ready) {
    mock_display_ready = ready;
    mock_hw_config.display_ready = ready;
}

// Mock implementations of hardware.h functions
bool hardware_init(HardwareConfig *config) {
    mock_hw_initialized = true;
    return true;
}

void hardware_deinit(void) {
    mock_hw_initialized = false;
}

HardwareConfig* hardware_get_config(void) {
    return mock_hw_initialized ? &mock_hw_config : NULL;
}

bool hardware_is_initialized(void) {
    return mock_hw_initialized;
}

bool hardware_is_display_ready(void) {
    return mock_display_ready;
} 