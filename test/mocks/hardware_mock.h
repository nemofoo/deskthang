#ifndef HARDWARE_MOCK_H
#define HARDWARE_MOCK_H

#include "hardware/hardware.h"

// Mock hardware functions
void mock_hardware_reset(void);
void mock_hardware_set_initialized(bool initialized);
void mock_hardware_set_display_ready(bool ready);

// Mock hardware state tracking
extern HardwareConfig mock_hw_config;
extern bool mock_hw_initialized;
extern bool mock_display_ready;

#endif 