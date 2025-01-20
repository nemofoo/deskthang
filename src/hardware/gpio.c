#include "gpio.h"
#include "hardware.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include <string.h>

// GPIO state tracking
static struct {
    uint8_t pin_mosi;
    uint8_t pin_sck;
    uint8_t pin_cs;
    uint8_t pin_dc;
    uint8_t pin_rst;
    bool initialized;
} gpio_state;

bool display_gpio_init(const HardwareConfig *config) {
    if (!config) {
        return false;
    }

    // Store pin configuration
    gpio_state.pin_mosi = config->pins.mosi;
    gpio_state.pin_sck = config->pins.sck;
    gpio_state.pin_cs = config->pins.cs;
    gpio_state.pin_dc = config->pins.dc;
    gpio_state.pin_rst = config->pins.rst;

    // Initialize GPIO pins
    gpio_init(gpio_state.pin_mosi);
    gpio_init(gpio_state.pin_sck);
    gpio_init(gpio_state.pin_cs);
    gpio_init(gpio_state.pin_dc);
    gpio_init(gpio_state.pin_rst);

    // Set pin directions
    gpio_set_dir(gpio_state.pin_mosi, GPIO_OUT);
    gpio_set_dir(gpio_state.pin_sck, GPIO_OUT);
    gpio_set_dir(gpio_state.pin_cs, GPIO_OUT);
    gpio_set_dir(gpio_state.pin_dc, GPIO_OUT);
    gpio_set_dir(gpio_state.pin_rst, GPIO_OUT);

    // Set initial pin states
    gpio_put(gpio_state.pin_cs, 1);  // CS inactive high
    gpio_put(gpio_state.pin_dc, 1);  // Data mode
    gpio_put(gpio_state.pin_rst, 1); // Not in reset

    // Configure SPI pins
    gpio_set_function(gpio_state.pin_mosi, GPIO_FUNC_SPI);
    gpio_set_function(gpio_state.pin_sck, GPIO_FUNC_SPI);

    gpio_state.initialized = true;
    return true;
}

void display_gpio_deinit(void) {
    if (!gpio_state.initialized) {
        return;
    }

    // Reset all pins to inputs with no pulls
    gpio_set_dir(gpio_state.pin_mosi, GPIO_IN);
    gpio_set_dir(gpio_state.pin_sck, GPIO_IN);
    gpio_set_dir(gpio_state.pin_cs, GPIO_IN);
    gpio_set_dir(gpio_state.pin_dc, GPIO_IN);
    gpio_set_dir(gpio_state.pin_rst, GPIO_IN);

    gpio_disable_pulls(gpio_state.pin_mosi);
    gpio_disable_pulls(gpio_state.pin_sck);
    gpio_disable_pulls(gpio_state.pin_cs);
    gpio_disable_pulls(gpio_state.pin_dc);
    gpio_disable_pulls(gpio_state.pin_rst);

    gpio_state.initialized = false;
}

void display_gpio_set(uint8_t pin, bool value) {
    if (!gpio_state.initialized) {
        return;
    }

    gpio_put(pin, value);
}
