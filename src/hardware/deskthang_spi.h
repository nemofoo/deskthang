#ifndef DESKTHANG_SPI_H
#define DESKTHANG_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"  // This provides spi_inst_t and SPI functions
#include "deskthang_gpio.h"  // Update if it's using gpio.h

// SPI Configuration structure
typedef struct {
    uint8_t spi_port;    // 0 or 1
    uint32_t baud_rate;  // Hz
    uint8_t cs_pin;      // Chip select pin
    uint8_t sck_pin;     // Clock pin
    uint8_t mosi_pin;    // MOSI pin
    uint8_t miso_pin;    // MISO pin
} DeskthangSPIConfig;

// SPI interface functions
bool deskthang_spi_init(const DeskthangSPIConfig *config);
void deskthang_spi_deinit(void);
bool deskthang_spi_write(const uint8_t *data, size_t len);
bool deskthang_spi_read(uint8_t *data, size_t len);
bool deskthang_spi_transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t len);
void deskthang_spi_chip_select(bool select);

// SPI status check
bool deskthang_spi_is_initialized(void);

#endif // DESKTHANG_SPI_H 