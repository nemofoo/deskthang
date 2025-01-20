#ifndef DESKTHANG_GPIO_H
#define DESKTHANG_GPIO_H

#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"  // For HardwareConfig

// GPIO initialization and configuration
bool deskthang_gpio_init(const HardwareConfig *config);
void deskthang_gpio_deinit(void);

// GPIO pin control
void deskthang_gpio_set(uint8_t pin, bool value);
bool deskthang_gpio_get(uint8_t pin);

// GPIO status check
bool deskthang_gpio_is_initialized(void);

#endif // DESKTHANG_GPIO_H 