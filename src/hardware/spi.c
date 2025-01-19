#include "spi.h"
#include "hardware/spi.h"

// Static configuration
static struct {
    spi_inst_t *spi;
    uint baud_rate;
    bool initialized;
} spi_state = {0};

bool display_spi_init(const HardwareConfig *config) {
    if (config == NULL) {
        return false;
    }

    // Select SPI instance based on port number
    spi_state.spi = (config->spi_port == 0) ? spi0 : spi1;
    spi_state.baud_rate = config->spi_baud;

    // Initialize SPI with requested baud rate
    spi_init(spi_state.spi, spi_state.baud_rate);

    // Configure SPI mode 0 (CPOL=0, CPHA=0)
    spi_set_format(spi_state.spi,
                   8,       // 8 bits per transfer
                   SPI_CPOL_0,  // Clock polarity
                   SPI_CPHA_0,  // Clock phase
                   SPI_MSB_FIRST); // MSB first

    spi_state.initialized = true;
    return true;
}

void display_spi_deinit(void) {
    if (!spi_state.initialized) {
        return;
    }

    // Deinitialize SPI
    spi_deinit(spi_state.spi);

    // Clear state
    spi_state.initialized = false;
}

size_t display_spi_write(const uint8_t *data, size_t len) {
    if (!spi_state.initialized || data == NULL || len == 0) {
        return 0;
    }

    // Write data over SPI
    return spi_write_blocking(spi_state.spi, data, len);
}

bool display_spi_write_byte(uint8_t byte) {
    if (!spi_state.initialized) {
        return false;
    }

    // Write single byte
    size_t written = spi_write_blocking(spi_state.spi, &byte, 1);
    return (written == 1);
}

size_t display_spi_read(uint8_t *data, size_t len) {
    if (!spi_state.initialized || data == NULL || len == 0) {
        return 0;
    }

    // Read data over SPI
    return spi_read_blocking(spi_state.spi, 0xFF, data, len);
}

bool display_spi_read_byte(uint8_t *byte) {
    if (!spi_state.initialized || byte == NULL) {
        return false;
    }

    // Read single byte
    size_t read = spi_read_blocking(spi_state.spi, 0xFF, byte, 1);
    return (read == 1);
}
