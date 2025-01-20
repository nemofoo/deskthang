#ifndef __DESKTHANG_SPI_H
#define __DESKTHANG_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware.h"

// SPI Configuration structure
typedef struct {
    uint8_t spi_port;    // 0 or 1
    uint32_t baud_rate;  // Hz
    uint8_t cs_pin;      // Chip select pin
    uint8_t sck_pin;     // Clock pin
    uint8_t mosi_pin;    // MOSI pin
    uint8_t miso_pin;    // MISO pin
} SPIConfig;

// SPI interface functions
bool spi_device_init(const SPIConfig *config);
void spi_device_deinit(void);
bool spi_write(const uint8_t *data, size_t len);
bool spi_read(uint8_t *data, size_t len);
bool spi_transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t len);
void spi_chip_select(bool select);

#endif // __DESKTHANG_SPI_H
