#ifndef __DESKTHANG_GPIO_H
#define __DESKTHANG_GPIO_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include "hardware/structs/iobank0.h"
#include "hardware/structs/padsbank0.h"

// GPIO callback function type
typedef void (*gpio_irq_callback_t)(uint gpio, uint32_t events);

// GPIO drive strength enumeration
enum gpio_drive_strength {
    GPIO_DRIVE_STRENGTH_2MA = 0,
    GPIO_DRIVE_STRENGTH_4MA = 1,
    GPIO_DRIVE_STRENGTH_8MA = 2,
    GPIO_DRIVE_STRENGTH_12MA = 3
};

// GPIO slew rate enumeration
enum gpio_slew_rate {
    GPIO_SLEW_RATE_SLOW = 0,
    GPIO_SLEW_RATE_FAST = 1
};

// GPIO direction constants
#define GPIO_IN  0
#define GPIO_OUT 1

// Parameter assertions
#define PARAM_ASSERTIONS_ENABLED_HARDWARE_GPIO 1

// GPIO IRQ callback priority
#define GPIO_IRQ_CALLBACK_ORDER_PRIORITY 0x40
#define GPIO_RAW_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY

// Debug pin configuration
#ifndef PICO_DEBUG_PIN_BASE
#define PICO_DEBUG_PIN_BASE 19
#endif

#ifndef PICO_DEBUG_PIN_COUNT
#define PICO_DEBUG_PIN_COUNT 3
#endif

// Core GPIO functions - match SDK types
void gpio_init(uint gpio);
void gpio_set_function(uint gpio, gpio_function_t fn);
void gpio_set_dir(uint gpio, bool out);
void gpio_put(uint gpio, bool value);
void gpio_set_dir_masked(uint32_t mask, uint32_t value);
void gpio_init_mask(uint gpio_mask);
void gpio_disable_pulls(uint gpio);
void gpio_set_irq_callback(gpio_irq_callback_t callback);
void gpio_acknowledge_irq(uint gpio, uint32_t events);
void gpio_set_irq_enabled(uint gpio, uint32_t events, bool enabled);
void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t events, bool enabled, gpio_irq_callback_t callback);
bool gpio_get(uint gpio);
void gpio_set_pulls(uint gpio, bool up, bool down);
void gpio_set_drive_strength(uint gpio, enum gpio_drive_strength drive);
void gpio_set_slew_rate(uint gpio, enum gpio_slew_rate slew);
void gpio_set_input_enabled(uint gpio, bool enabled);
void gpio_set_input_hysteresis_enabled(uint gpio, bool enabled);

// Our display GPIO interface functions
bool display_gpio_init(const HardwareConfig *config);
void display_gpio_deinit(void);
void display_gpio_set(uint8_t pin, bool value);

// SPI interface functions
bool display_spi_init(const HardwareConfig *config);
void display_spi_deinit(void);
size_t display_spi_write(const uint8_t *data, size_t len);
bool display_spi_write_byte(uint8_t byte);

#endif // __DESKTHANG_GPIO_H
