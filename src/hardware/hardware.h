#ifndef __DESKTHANG_HARDWARE_H
#define __DESKTHANG_HARDWARE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Pin configuration structure
typedef struct {
    uint8_t mosi;      // SPI MOSI pin
    uint8_t sck;       // SPI clock pin
    uint8_t cs;        // Chip select pin
    uint8_t dc;        // Data/command pin
    uint8_t rst;       // Reset pin
} PinConfig;

// Hardware configuration structure
typedef struct {
    // Core configuration
    uint8_t spi_port;      // SPI port number
    uint32_t spi_baud;     // Baud rate (Hz)
    
    // Pin assignments
    PinConfig pins;
    
    // Timing parameters
    struct {
        uint32_t reset_pulse_us;    // Reset pulse width
        uint32_t init_delay_ms;     // Post-init delay
        uint32_t cmd_delay_us;      // Command delay
    } timing;
    
    // Status flags
    bool initialized;              // Hardware init complete
    bool display_ready;           // Display init complete
} HardwareConfig;

/**
 * Initialize hardware subsystems
 * @param config Hardware configuration
 * @return true if initialization successful, false otherwise
 */
bool hardware_init(HardwareConfig *config);

/**
 * Deinitialize hardware subsystems
 */
void hardware_deinit(void);

/**
 * Get current hardware configuration
 * @return Pointer to active hardware configuration
 */
HardwareConfig* hardware_get_config(void);

/**
 * Check if hardware is initialized
 * @return true if hardware is initialized, false otherwise
 */
bool hardware_is_initialized(void);

/**
 * Check if display is ready
 * @return true if display is ready, false otherwise
 */
bool hardware_is_display_ready(void);

/**
 * Reset hardware subsystems
 * @return true if reset successful, false otherwise
 */
bool hardware_reset(void);

#ifdef __cplusplus
}
#endif

#endif // __DESKTHANG_HARDWARE_H
