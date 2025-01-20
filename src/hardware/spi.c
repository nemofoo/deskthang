#include "spi.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"  // Add this for spi_inst_t and SPI functions
#include "hardware/gpio.h"  // Add this for GPIO functions
#include "../error/logging.h"

// Static configuration
static struct {
    spi_inst_t *spi;
    uint32_t baud_rate;
    uint8_t cs_pin;
    uint8_t sck_pin;
    uint8_t mosi_pin;
    uint8_t miso_pin;
    bool initialized;
} spi_state = {0};

bool spi_device_init(const SPIConfig *config) {
    if (!config) {
        return false;
    }

    // Select SPI instance
    spi_state.spi = (config->spi_port == 0) ? spi0 : spi1;
    spi_state.baud_rate = config->baud_rate;
    spi_state.cs_pin = config->cs_pin;
    spi_state.sck_pin = config->sck_pin;
    spi_state.mosi_pin = config->mosi_pin;
    spi_state.miso_pin = config->miso_pin;

    // Initialize SPI pins
    gpio_set_function(spi_state.sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_state.mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_state.miso_pin, GPIO_FUNC_SPI);
    gpio_init(spi_state.cs_pin);
    gpio_set_dir(spi_state.cs_pin, GPIO_OUT);
    gpio_put(spi_state.cs_pin, 1);  // CS high (inactive)

    // Initialize SPI hardware
    spi_init(spi_state.spi, spi_state.baud_rate);
    
    // Configure SPI format (8 bits per transfer, SPI mode 0, MSB first)
    uint actual = spi_set_baudrate(spi_state.spi, spi_state.baud_rate);
    spi_set_format(spi_state.spi, 
                   8,                    // 8 bits per transfer
                   SPI_CPOL_0,          // Clock polarity
                   SPI_CPHA_0,          // Clock phase
                   SPI_MSB_FIRST);      // MSB first

    spi_state.initialized = true;
    return true;
}

void spi_device_deinit(void) {
    if (!spi_state.initialized) {
        return;
    }

    spi_deinit(spi_state.spi);
    gpio_set_function(spi_state.sck_pin, GPIO_FUNC_NULL);
    gpio_set_function(spi_state.mosi_pin, GPIO_FUNC_NULL);
    gpio_set_function(spi_state.miso_pin, GPIO_FUNC_NULL);
    gpio_init(spi_state.cs_pin);  // Reset CS pin
    spi_state.initialized = false;
}

bool spi_write(const uint8_t *data, size_t len) {
    if (!spi_state.initialized || !data) {
        return false;
    }

    gpio_put(spi_state.cs_pin, 0);  // CS low (active)
    int bytes_written = spi_write_blocking(spi_state.spi, data, len);
    gpio_put(spi_state.cs_pin, 1);  // CS high (inactive)

    return bytes_written == len;
}

bool spi_read(uint8_t *data, size_t len) {
    if (!spi_state.initialized || !data) {
        return false;
    }

    gpio_put(spi_state.cs_pin, 0);  // CS low (active)
    int bytes_read = spi_read_blocking(spi_state.spi, 0xFF, data, len);
    gpio_put(spi_state.cs_pin, 1);  // CS high (inactive)

    return bytes_read == len;
}

bool spi_transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t len) {
    if (!spi_state.initialized || !tx_data || !rx_data) {
        return false;
    }

    gpio_put(spi_state.cs_pin, 0);  // CS low (active)
    int bytes_transferred = spi_write_read_blocking(spi_state.spi, tx_data, rx_data, len);
    gpio_put(spi_state.cs_pin, 1);  // CS high (inactive)

    return bytes_transferred == len;
}

void spi_chip_select(bool select) {
    if (!spi_state.initialized) {
        return;
    }

    gpio_put(spi_state.cs_pin, !select); // CS is active low
}
