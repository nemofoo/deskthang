#ifndef DESKTHANG_GPIO_H
#define DESKTHANG_GPIO_H

#include "hardware.h"

bool display_gpio_init(const HardwareConfig *config);
void display_gpio_deinit(void);
void display_gpio_set(uint8_t pin, bool value);

#endif // DESKTHANG_GPIO_H 