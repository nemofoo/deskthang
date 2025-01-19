#ifndef __DESKTHANG_GPIO_H
#define __DESKTHANG_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware.h"
#include "hardware/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize GPIO pins for display interface
 * @param config Hardware configuration containing pin assignments
 * @return true if initialization successful, false otherwise
 */
bool display_gpio_init(const HardwareConfig *config);

/**
 * Deinitialize display GPIO pins
 */
void display_gpio_deinit(void);

/**
 * Set display control pin value
 * @param pin Pin number from HardwareConfig
 * @param value Pin value (0 or 1)
 */
void display_gpio_set(uint pin, bool value);

#ifdef __cplusplus
}
#endif

#endif // __DESKTHANG_GPIO_H
