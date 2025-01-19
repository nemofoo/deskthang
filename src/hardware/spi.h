#ifndef __DESKTHANG_SPI_H
#define __DESKTHANG_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware.h"
#include "pico-sdk/src/rp2_common/hardware_spi/include/hardware/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize SPI interface for display
 * @param config Hardware configuration containing SPI settings
 * @return true if initialization successful, false otherwise
 */
bool display_spi_init(const HardwareConfig *config);

/**
 * Deinitialize display SPI interface
 */
void display_spi_deinit(void);

/**
 * Write data to display over SPI
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 * @return Number of bytes written
 */
size_t display_spi_write(const uint8_t *data, size_t len);

/**
 * Write single byte to display over SPI
 * @param byte Byte to write
 * @return true if write successful, false otherwise
 */
bool display_spi_write_byte(uint8_t byte);

/**
 * Read data from display over SPI
 * @param data Pointer to data buffer
 * @param len Length of data to read in bytes
 * @return Number of bytes read
 */
size_t display_spi_read(uint8_t *data, size_t len);

/**
 * Read single byte from display over SPI
 * @param byte Pointer to store read byte
 * @return true if read successful, false otherwise
 */
bool display_spi_read_byte(uint8_t *byte);

#ifdef __cplusplus
}
#endif

#endif // __DESKTHANG_SPI_H
