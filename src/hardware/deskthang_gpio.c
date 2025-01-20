#include "deskthang_gpio.h"
#include "hardware.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"  // Pico SDK GPIO

// Add at the top with other static variables
static bool gpio_initialized = false;

bool deskthang_gpio_init(const HardwareConfig *config) {
    if (!config) {
        return false;
    }

    // Set all pins as outputs
    gpio_set_dir(config->pins.rst, GPIO_OUT);
    gpio_set_dir(config->pins.dc, GPIO_OUT);
    gpio_set_dir(config->pins.cs, GPIO_OUT);
    gpio_set_dir(config->pins.sck, GPIO_OUT);
    gpio_set_dir(config->pins.mosi, GPIO_OUT);

    // Set initial pin states
    gpio_put(config->pins.rst, 1);  // Reset high
    gpio_put(config->pins.dc, 1);   // Data mode
    gpio_put(config->pins.cs, 1);   // Not selected

    gpio_initialized = true;
    return true;
}

void deskthang_gpio_deinit(void) {
    // Reset all pins to inputs with no pulls
    const HardwareConfig *config = hardware_get_config();
    if (!config) {
        return;
    }

    gpio_set_dir(config->pins.rst, GPIO_IN);
    gpio_set_dir(config->pins.dc, GPIO_IN);
    gpio_set_dir(config->pins.cs, GPIO_IN);
    gpio_set_dir(config->pins.sck, GPIO_IN);
    gpio_set_dir(config->pins.mosi, GPIO_IN);

    // Disable pulls
    gpio_disable_pulls(config->pins.rst);
    gpio_disable_pulls(config->pins.dc);
    gpio_disable_pulls(config->pins.cs);
    gpio_disable_pulls(config->pins.sck);
    gpio_disable_pulls(config->pins.mosi);

    gpio_initialized = false;
}

void deskthang_gpio_set(uint8_t pin, bool value) {
    gpio_put(pin, value);
}

bool deskthang_gpio_get(uint8_t pin) {
    return gpio_get(pin);
}

bool deskthang_gpio_is_initialized(void) {
    return gpio_initialized;
} 