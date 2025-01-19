#ifndef __DESKTHANG_DISPLAY_H
#define __DESKTHANG_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware.h"
#include "GC9A01.h"

#ifdef __cplusplus
extern "C" {
#endif

// Display dimensions
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240

// Color depth
#define DISPLAY_COLOR_DEPTH 16  // 16-bit color (RGB565)

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

#ifdef __cplusplus
}
#endif

#endif // __DESKTHANG_DISPLAY_H
