#include "../system/time.h"
#include "display.h"
#include "deskthang_gpio.h"
#include "deskthang_spi.h"
#include "GC9A01.h"
#include <string.h>

// Static configuration
static struct {
    DisplayConfig config;
    bool initialized;
} display_state = {0};

// Static hardware configuration
static const HardwareConfig *hw_config = NULL;

// Add at the top with other static variables
static bool display_initialized = false;
static uint8_t display_status = 0;
static uint16_t display_buffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];
static size_t buffer_used = 0;

// GC9A01 HAL function implementations
void GC9A01_set_reset(uint8_t val) {
    if (hw_config) {
        deskthang_gpio_set(hw_config->pins.rst, val);
    }
}

void GC9A01_set_data_command(uint8_t val) {
    if (hw_config) {
        deskthang_gpio_set(hw_config->pins.dc, val);
    }
}

void GC9A01_set_chip_select(uint8_t val) {
    if (hw_config) {
        deskthang_gpio_set(hw_config->pins.cs, val);
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
    display_initialized = true;
    display_status = 0;
    buffer_used = 0;
    
    return true;
}

void display_deinit(void) {
    if (!display_state.initialized) {
        return;
    }

    // Clear state
    memset(&display_state, 0, sizeof(display_state));
    
    display_initialized = false;
    display_status = 0;
    buffer_used = 0;
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
    return display_initialized;
}

bool display_reset_complete(void) {
    // TODO: Implement reset completion check
    return true;
}

bool display_params_valid(void) {
    if (!display_initialized) {
        return false;
    }
    
    // Check display configuration parameters
    uint8_t display_mode = GC9A01_read_display_mode();
    if ((display_mode & GC9A01_MODE_VALID_MASK) != GC9A01_MODE_EXPECTED) {
        return false;
    }
    
    // Check memory access mode
    uint8_t mem_access = GC9A01_read_memory_access();
    if ((mem_access & GC9A01_MEM_ACCESS_MASK) != GC9A01_MEM_ACCESS_EXPECTED) {
        return false;
    }
    
    return true;
}

bool display_responding(void) {
    // TODO: Implement display response check
    return true;
}

bool display_draw_test_pattern(TestPattern pattern, uint16_t color) {
    if (!display_state.initialized) {
        return false;
    }

    switch (pattern) {
        case TEST_PATTERN_COLOR_BARS:
            return display_draw_color_bars();
        case TEST_PATTERN_GRADIENT:
            return display_draw_gradient();
        case TEST_PATTERN_CHECKERBOARD:
            return display_draw_checkerboard(20); // Default 20px squares
        case TEST_PATTERN_SOLID:
            return display_fill_solid(color);
        default:
            return false;
    }
}

bool display_draw_color_bars(void) {
    if (!display_state.initialized) {
        return false;
    }

    const uint16_t colors[] = {
        COLOR_RED,
        COLOR_GREEN,
        COLOR_BLUE,
        COLOR_YELLOW,
        COLOR_MAGENTA,
        COLOR_CYAN,
        COLOR_WHITE,
        COLOR_BLACK
    };
    const uint8_t num_bars = sizeof(colors) / sizeof(colors[0]);
    const uint16_t bar_width = DISPLAY_WIDTH / num_bars;

    for (uint8_t i = 0; i < num_bars; i++) {
        uint16_t x = i * bar_width;
        GC9A01_fill_rect(x, 0, bar_width, DISPLAY_HEIGHT, colors[i]);
    }

    return true;
}

bool display_draw_gradient(void) {
    if (!display_state.initialized) {
        return false;
    }

    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y++) {
        for (uint16_t x = 0; x < DISPLAY_WIDTH; x++) {
            // Calculate RGB components based on position
            uint8_t r = (x * 255) / DISPLAY_WIDTH;
            uint8_t g = (y * 255) / DISPLAY_HEIGHT;
            uint8_t b = ((x + y) * 255) / (DISPLAY_WIDTH + DISPLAY_HEIGHT);
            
            uint16_t color = RGB565(r, g, b);
            GC9A01_draw_pixel(x, y, color);
        }
    }

    return true;
}

bool display_draw_checkerboard(uint8_t square_size) {
    if (!display_state.initialized || square_size == 0) {
        return false;
    }

    for (uint16_t y = 0; y < DISPLAY_HEIGHT; y += square_size) {
        for (uint16_t x = 0; x < DISPLAY_WIDTH; x += square_size) {
            uint16_t color = ((x / square_size + y / square_size) % 2) ? COLOR_BLACK : COLOR_WHITE;
            uint16_t width = (x + square_size > DISPLAY_WIDTH) ? DISPLAY_WIDTH - x : square_size;
            uint16_t height = (y + square_size > DISPLAY_HEIGHT) ? DISPLAY_HEIGHT - y : square_size;
            GC9A01_fill_rect(x, y, width, height, color);
        }
    }

    return true;
}

bool display_fill_solid(uint16_t color) {
    if (!display_state.initialized) {
        return false;
    }

    GC9A01_fill_rect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, color);
    return true;
}

// Display status checks
bool display_is_responding(void) {
    if (!display_initialized) {
        return false;
    }
    
    // Read display status register
    uint8_t status = GC9A01_read_status();
    display_status = status;
    
    // Check if display is ready
    return (status & GC9A01_STATUS_READY) != 0;
}

bool display_buffer_available(void) {
    return buffer_used < sizeof(display_buffer);
}

// Update buffer management functions
void display_update_buffer_usage(size_t bytes_used) {
    buffer_used = bytes_used;
}
