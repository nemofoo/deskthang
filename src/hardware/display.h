#ifndef __DESKTHANG_DISPLAY_H
#define __DESKTHANG_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../common/deskthang_constants.h"
#include "hardware.h"
#include "GC9A01.h"
#include "deskthang_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display dimensions
// #define DISPLAY_WIDTH  240
// #define DISPLAY_HEIGHT 240

// Color depth
// #define DISPLAY_COLOR_DEPTH 16  // 16-bit color (RGB565)

// Display orientation options
typedef enum {
    DISPLAY_ORIENTATION_0   = ORIENTATION_0,   // 0 degrees
    DISPLAY_ORIENTATION_90  = ORIENTATION_90,  // 90 degrees clockwise
    DISPLAY_ORIENTATION_180 = ORIENTATION_180, // 180 degrees
    DISPLAY_ORIENTATION_270 = ORIENTATION_270  // 270 degrees clockwise
} DisplayOrientation;

// Display configuration
typedef struct {
    DisplayOrientation orientation;  // Display orientation
    uint8_t brightness;             // Display brightness (0-255)
    bool inverted;                  // Invert display colors
} DisplayConfig;

/**
 * Initialize display hardware and controller
 * @param hw_config Hardware configuration
 * @param disp_config Display configuration
 * @return true if initialization successful, false otherwise
 */
bool display_init(const HardwareConfig *hw_config, const DisplayConfig *disp_config);

/**
 * Deinitialize display
 */
void display_deinit(void);

/**
 * Set display orientation
 * @param orientation New display orientation
 * @return true if orientation change successful, false otherwise
 */
bool display_set_orientation(DisplayOrientation orientation);

/**
 * Set display brightness
 * @param brightness Brightness level (0-255)
 * @return true if brightness change successful, false otherwise
 */
bool display_set_brightness(uint8_t brightness);

/**
 * Set display color inversion
 * @param inverted true to invert colors, false for normal colors
 * @return true if inversion change successful, false otherwise
 */
bool display_set_inverted(bool inverted);

/**
 * Write pixel data to display
 * @param x X coordinate (0 to DISPLAY_WIDTH-1)
 * @param y Y coordinate (0 to DISPLAY_HEIGHT-1)
 * @param width Width of pixel data
 * @param height Height of pixel data
 * @param data Pixel data buffer (RGB565 format)
 * @return true if write successful, false otherwise
 */
bool display_write_pixels(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *data);

/**
 * Fill display region with solid color
 * @param x X coordinate (0 to DISPLAY_WIDTH-1)
 * @param y Y coordinate (0 to DISPLAY_HEIGHT-1)
 * @param width Width of region
 * @param height Height of region
 * @param color Color in RGB565 format
 * @return true if fill successful, false otherwise
 */
bool display_fill_region(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);

/**
 * Clear display (fill with black)
 * @return true if clear successful, false otherwise
 */
bool display_clear(void);

/**
 * Get current display configuration
 * @return Pointer to active display configuration
 */
const DisplayConfig* display_get_config(void);

/**
 * Check if display is initialized
 * @return true if display is initialized, false otherwise
 */
bool display_is_initialized(void);

// Add these declarations
bool display_reset_complete(void);
bool display_params_valid(void);
bool display_responding(void);

/**
 * Test pattern types
 */
typedef enum {
    TEST_PATTERN_COLOR_BARS,    // Vertical color bars
    TEST_PATTERN_GRADIENT,      // Diagonal RGB gradient
    TEST_PATTERN_CHECKERBOARD,  // Black and white checkerboard
    TEST_PATTERN_SOLID         // Solid color fill
} TestPattern;

/**
 * Draw a test pattern on the display
 * @param pattern The test pattern to draw
 * @param color Optional color parameter for solid fill (ignored for other patterns)
 * @return true if successful, false otherwise
 */
bool display_draw_test_pattern(TestPattern pattern, uint16_t color);

/**
 * Draw vertical color bars
 * Shows basic RGB colors in vertical stripes
 * @return true if successful, false otherwise
 */
bool display_draw_color_bars(void);

/**
 * Draw diagonal RGB gradient
 * Shows smooth transition between primary colors
 * @return true if successful, false otherwise
 */
bool display_draw_gradient(void);

/**
 * Draw checkerboard pattern
 * Shows alternating black and white squares
 * @param square_size Size of each square in pixels
 * @return true if successful, false otherwise
 */
bool display_draw_checkerboard(uint8_t square_size);

/**
 * Fill display with solid color
 * @param color 16-bit RGB565 color value
 * @return true if successful, false otherwise
 */
bool display_fill_solid(uint16_t color);

// Display status checks
bool display_is_responding(void);
bool display_buffer_available(void);

// Display operations
void display_update(void);
void display_set_pixel(uint16_t x, uint16_t y, uint16_t color);

// Display status functions
bool display_ready(void);

// Display write functions
bool display_write_data(const uint8_t *data, uint32_t len);
bool display_end_write(void);

#ifdef __cplusplus
}
#endif

#endif // __DESKTHANG_DISPLAY_H
