#include <stdio.h>
#include "pico/stdlib.h"    // Add this for stdio_init_all() and sleep_ms()
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "GC9A01.h"
#include "colors.h"

#define SPI_BAUD 10000000  // 10MHz

// Pin definitions
#define PIN_MOSI 19
#define PIN_SCK  18
#define PIN_CS   17
#define PIN_DC   16
#define PIN_RST  20
#define SPI_PORT spi0

// Implementation of the hardware abstraction layer
void GC9A01_set_reset(uint8_t val) {
    gpio_put(PIN_RST, val);
}

void GC9A01_set_data_command(uint8_t val) {
    gpio_put(PIN_DC, val);
}

void GC9A01_set_chip_select(uint8_t val) {
    gpio_put(PIN_CS, val);
}

void GC9A01_spi_tx(uint8_t *data, size_t len) {
    spi_write_blocking(SPI_PORT, data, len);
}

void GC9A01_delay(uint16_t ms) {
    sleep_ms(ms);
}

void display_init(void) {
    // Initialize SPI pins
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    
    gpio_put(PIN_CS, 1);
    gpio_put(PIN_DC, 1);
    
    spi_init(SPI_PORT, SPI_BAUD);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    GC9A01_init();  // This will use the Arduino initialization sequence
}

void test_pattern(void) {
    struct GC9A01_frame frame = {{0,0}, {239,239}};
    GC9A01_set_frame(frame);
    
    uint16_t colors[] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE};
    uint8_t color_bytes[2];
    
    for(int y = 0; y < 240; y++) {
        uint16_t current_color = colors[y / 80];  // Change color every 80 pixels
        color_bytes[0] = current_color >> 8;      // High byte
        color_bytes[1] = current_color & 0xFF;    // Low byte
        
        for(int x = 0; x < 240; x++) {
            if(x == 0 && y == 0) {
                GC9A01_write(color_bytes, 2);
            } else {
                GC9A01_write_continue(color_bytes, 2);
            }
        }
    }
}

int main() {
    stdio_init_all();
    display_init();
    
    while(1) {
        test_pattern();
        sleep_ms(1000);
    }
    
    return 0;
}
