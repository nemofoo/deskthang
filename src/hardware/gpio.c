#include "gpio.h"
#include "hardware/gpio.h"

// Static configuration
static struct {
    uint pin_mosi;
    uint pin_sck;
    uint pin_cs;
    uint pin_dc;
    uint pin_rst;
    bool initialized;
} gpio_state = {0};

bool display_gpio_init(const HardwareConfig *config) {
    if (config == NULL) {
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

    // Set SPI pin functions
    gpio_set_function(gpio_state.pin_mosi, GPIO_FUNC_SPI);
    gpio_set_function(gpio_state.pin_sck, GPIO_FUNC_SPI);

    gpio_state.initialized = true;
    return true;
}

void display_gpio_deinit(void) {
    if (!gpio_state.initialized) {
        return;
    }

    // Reset all pins to inputs (safe state)
    gpio_set_dir(gpio_state.pin_mosi, GPIO_IN);
    gpio_set_dir(gpio_state.pin_sck, GPIO_IN);
    gpio_set_dir(gpio_state.pin_cs, GPIO_IN);
    gpio_set_dir(gpio_state.pin_dc, GPIO_IN);
    gpio_set_dir(gpio_state.pin_rst, GPIO_IN);

    // Disable pull-ups/downs
    gpio_disable_pulls(gpio_state.pin_mosi);
    gpio_disable_pulls(gpio_state.pin_sck);
    gpio_disable_pulls(gpio_state.pin_cs);
    gpio_disable_pulls(gpio_state.pin_dc);
    gpio_disable_pulls(gpio_state.pin_rst);

    // Reset pin functions to GPIO
    gpio_set_function(gpio_state.pin_mosi, GPIO_FUNC_NULL);
    gpio_set_function(gpio_state.pin_sck, GPIO_FUNC_NULL);

    // Clear state
    gpio_state.initialized = false;
}

void display_gpio_set(uint pin, bool value) {
    if (!gpio_state.initialized) {
        return;
    }

    // Only allow setting control pins (CS, DC, RST)
    if (pin == gpio_state.pin_cs ||
        pin == gpio_state.pin_dc ||
        pin == gpio_state.pin_rst) {
        gpio_put(pin, value);
    }
}
