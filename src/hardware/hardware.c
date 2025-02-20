#include "hardware.h"
#include "deskthang_gpio.h"
#include "deskthang_spi.h"
#include "display.h"
#include "../error/logging.h"
#include <string.h>
#include <stdio.h>

// Static hardware configuration
static HardwareConfig hw_config;
static bool is_initialized = false;

bool hardware_init(const HardwareConfig *config) {
    if (config == NULL) {
        logging_write("Hardware", "NULL config provided");
        return false;
    }

    // Store configuration in a non-const local copy
    memcpy(&hw_config, config, sizeof(HardwareConfig));
    
    logging_write("Hardware", "Initializing GPIO...");
    // Initialize GPIO first
    if (!deskthang_gpio_init(&hw_config)) {
        logging_write("Hardware", "GPIO initialization failed");
        return false;
    }
    logging_write("Hardware", "GPIO initialized successfully");
    
    char spi_msg[100];
    snprintf(spi_msg, sizeof(spi_msg), "Initializing SPI (port %d, baud %lu)...", 
           config->spi_port, (unsigned long)config->spi_baud);
    logging_write("Hardware", spi_msg);
    
    // Initialize SPI after GPIO
    DeskthangSPIConfig spi_config = {
        .spi_port = config->spi_port,
        .baud_rate = config->spi_baud,
        .cs_pin = config->pins.cs,
        .sck_pin = config->pins.sck,
        .mosi_pin = config->pins.mosi,
        .miso_pin = config->pins.miso
    };
    
    if (!deskthang_spi_init(&spi_config)) {
        logging_write("Hardware", "SPI initialization failed");
        deskthang_gpio_deinit();
        return false;
    }
    logging_write("Hardware", "SPI initialized successfully");
    
    // Mark as initialized
    is_initialized = true;
    hw_config.initialized = true;
    
    return true;
}

void hardware_deinit(void) {
    if (!is_initialized) {
        return;
    }
    
    // Deinitialize in reverse order
    deskthang_spi_deinit();
    deskthang_gpio_deinit();
    
    // Clear state
    memset(&hw_config, 0, sizeof(HardwareConfig));
    is_initialized = false;
}

HardwareConfig* hardware_get_config(void) {
    if (!is_initialized) {
        return NULL;
    }
    return &hw_config;
}

bool hardware_is_initialized(void) {
    return is_initialized;
}

bool hardware_is_display_ready(void) {
    if (!is_initialized) {
        return false;
    }
    return hw_config.display_ready;
}

bool hardware_reset(void) {
    if (!is_initialized) {
        return false;
    }
    
    // Store current config
    HardwareConfig temp_config;
    memcpy(&temp_config, &hw_config, sizeof(HardwareConfig));
    
    // Perform full reset
    hardware_deinit();
    return hardware_init(&temp_config);
}

bool spi_is_configured(void) {
    // TODO: Implement SPI configuration check
    return true;
}

bool gpio_pins_configured(void) {
    // TODO: Implement GPIO configuration check
    return true;
}

bool timing_requirements_met(void) {
    // TODO: Implement timing requirements check
    return true;
}
