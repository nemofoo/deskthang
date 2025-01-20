#include "deskthang_spi.h"
#include "hardware/spi.h"  // Pico SDK SPI
#include "hardware/gpio.h" // Pico SDK GPIO
#include "deskthang_gpio.h"
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

bool deskthang_spi_init(const DeskthangSPIConfig *config) {
    if (!config) {
        return false;
    }

    // Select SPI instance based on port number
    spi_state.spi = config->spi_port == 0 ? spi0 : spi1;
    spi_state.baud_rate = config->baud_rate;
    spi_state.cs_pin = config->cs_pin;
    spi_state.sck_pin = config->sck_pin;
    spi_state.mosi_pin = config->mosi_pin;
    spi_state.miso_pin = config->miso_pin;

    // Initialize SPI pins
    gpio_set_function(spi_state.sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_state.mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_state.miso_pin, GPIO_FUNC_SPI);
    
    // Initialize CS pin as GPIO
    gpio_init(spi_state.cs_pin);
    gpio_set_dir(spi_state.cs_pin, GPIO_OUT);
    gpio_put(spi_state.cs_pin, 1);  // CS high (inactive)

    // Initialize SPI hardware with default format
    spi_init(spi_state.spi, spi_state.baud_rate);
    
    // Set standard SPI format (mode 0, MSB first)
    spi_set_format(spi_state.spi,
                   8,       // 8 data bits
                   0,       // CPOL = 0
                   0,       // CPHA = 0
                   SPI_MSB_FIRST);

    spi_state.initialized = true;
    return true;
}

void deskthang_spi_deinit(void) {
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

bool deskthang_spi_write(const uint8_t *data, size_t len) {
    if (!spi_state.initialized || !data) {
        return false;
    }

    gpio_put(spi_state.cs_pin, 0);  // CS low (active)
    int bytes_written = spi_write_blocking(spi_state.spi, data, len);
    gpio_put(spi_state.cs_pin, 1);  // CS high (inactive)

    return bytes_written == len;
}

bool deskthang_spi_read(uint8_t *data, size_t len) {
    if (!spi_state.initialized || !data) {
        return false;
    }

    gpio_put(spi_state.cs_pin, 0);  // CS low (active)
    int bytes_read = spi_read_blocking(spi_state.spi, 0xFF, data, len);
    gpio_put(spi_state.cs_pin, 1);  // CS high (inactive)

    return bytes_read == len;
}

bool deskthang_spi_transfer(const uint8_t *tx_data, uint8_t *rx_data, size_t len) {
    if (!spi_state.initialized || !tx_data || !rx_data) {
        return false;
    }

    gpio_put(spi_state.cs_pin, 0);  // CS low (active)
    int bytes_transferred = spi_write_read_blocking(spi_state.spi, tx_data, rx_data, len);
    gpio_put(spi_state.cs_pin, 1);  // CS high (inactive)

    return bytes_transferred == len;
}

void deskthang_spi_chip_select(bool select) {
    if (!spi_state.initialized) {
        return;
    }

    gpio_put(spi_state.cs_pin, !select); // CS is active low
} 