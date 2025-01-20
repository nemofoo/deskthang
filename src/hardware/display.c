#include "../system/time.h"
#include "display.h"
#include "deskthang_gpio.h"
#include "deskthang_spi.h"
#include <string.h>

// Static configuration
static struct {
    DisplayConfig config;
    bool initialized;
} display_state = {0};

// Static hardware configuration
static const HardwareConfig *hw_config = NULL;

// GC9A01 HAL function implementations
void GC9A01_set_reset(uint8_t val) {
    if (hw_config) {
        display_gpio_set(hw_config->pins.rst, val);
    }
}

void GC9A01_set_data_command(uint8_t val) {
    if (hw_config) {
        display_gpio_set(hw_config->pins.dc, val);
    }
}

void GC9A01_set_chip_select(uint8_t val) {
    if (hw_config) {
        display_gpio_set(hw_config->pins.cs, val);
    }
}

void GC9A01_spi_tx(uint8_t *data, size_t len) {
    deskthang_spi_write(data, len);
}

bool display_init(const HardwareConfig *hw_config_in, const DisplayConfig *disp_config) {
    if (hw_config_in == NULL || disp_config == NULL) {
        return false;
    }

    // Store configurations
    hw_config = hw_config_in;
    memcpy(&display_state.config, disp_config, sizeof(DisplayConfig));

    // Initialize GC9A01
    GC9A01_init();

    // Configure display based on settings
    if (!display_set_orientation(disp_config->orientation)) {
        return false;
    }

    if (!display_set_brightness(disp_config->brightness)) {
        return false;
    }

    if (!display_set_inverted(disp_config->inverted)) {
        return false;
    }

    // Clear display to black
    if (!display_clear()) {
        return false;
    }

    display_state.initialized = true;
    return true;
}

void display_deinit(void) {
    if (!display_state.initialized) {
        return;
    }

    // Clear state
    memset(&display_state, 0, sizeof(display_state));
}

bool display_set_orientation(DisplayOrientation orientation) {
    if (!display_state.initialized) {
        return false;
    }

    // Update stored configuration
    display_state.config.orientation = orientation;

    // Send orientation command to display
    GC9A01_write_command(0x36); // MADCTL
    uint8_t madctl = orientation;
    GC9A01_spi_tx(&madctl, 1);

    return true;
}

bool display_set_brightness(uint8_t brightness) {
    if (!display_state.initialized) {
        return false;
    }

    // Update stored configuration
    display_state.config.brightness = brightness;

    // Send brightness command to display
    GC9A01_write_command(0x51); // Write brightness
    GC9A01_spi_tx(&brightness, 1);

    return true;
}

bool display_set_inverted(bool inverted) {
    if (!display_state.initialized) {
        return false;
    }

    // Update stored configuration
    display_state.config.inverted = inverted;

    // Send inversion command to display
    GC9A01_write_command(inverted ? 0x21 : 0x20); // INVON/INVOFF

    return true;
}

bool display_write_pixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *data) {
    if (!display_state.initialized || data == NULL) {
        return false;
    }

    // Set up write window
    struct GC9A01_frame frame = {
        .start = {x, y},
        .end = {x + width - 1, y + height - 1}
    };
    GC9A01_set_frame(frame);

    // Write pixel data using deskthang_spi_write
    deskthang_spi_write(data, width * height * 2); // 2 bytes per pixel (RGB565)

    return true;
}

bool display_fill_region(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color) {
    if (!display_state.initialized) {
        return false;
    }

    // Set up write window
    struct GC9A01_frame frame = {
        .start = {x, y},
        .end = {x + width - 1, y + height - 1}
    };
    GC9A01_set_frame(frame);

    // Fill with color
    uint8_t color_bytes[2] = {color >> 8, color & 0xFF};
    for (uint32_t i = 0; i < width * height; i++) {
        GC9A01_write(color_bytes, 2);
    }

    return true;
}

bool display_clear(void) {
    return display_fill_region(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 0x0000); // Black
}

const DisplayConfig* display_get_config(void) {
    if (!display_state.initialized) {
        return NULL;
    }
    return &display_state.config;
}

bool display_is_initialized(void) {
    return display_state.initialized;
}

bool display_reset_complete(void) {
    // TODO: Implement reset completion check
    return true;
}

bool display_params_valid(void) {
    // TODO: Implement parameter validation
    return true;
}

bool display_responding(void) {
    // TODO: Implement display response check
    return true;
}
