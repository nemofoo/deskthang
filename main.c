#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdio/driver.h"
#include "pico/stdio_usb.h"
#include "GC9A01.h"
#include "colors.h"

#define SPI_BAUD 10000000  // 10MHz
#define IMAGE_SIZE (240 * 240 * 2)  // 240x240 pixels, 2 bytes per pixel (RGB565)

// Pin definitions
#define PIN_MOSI 19
#define PIN_SCK  18
#define PIN_CS   17
#define PIN_DC   16
#define PIN_RST  20
#define SPI_PORT spi0

void debug_print_hex(uint8_t byte) {
    printf("Received: 0x%02X\n", byte);
}

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
    
    GC9A01_init();
}

void display_raw_image(void) {
    printf("Starting image receive process\n");
    
    struct GC9A01_frame frame = {{0,0}, {239,239}};
    GC9A01_set_frame(frame);
    
    // Start the write operation
    GC9A01_write_command(GC9A01_MEM_WR);
    
    // Buffer for receiving image data
    uint8_t buffer[512];
    size_t total_received = 0;
    
    printf("Waiting for IMAGE_COMMAND\n");
    int cmd = getchar_timeout_us(1000000);
    if (cmd == PICO_ERROR_TIMEOUT || cmd != 'I') {
        printf("Invalid or missing IMAGE_COMMAND: 0x%02X\n", cmd);
        return;
    }
    putchar('A');  // ACK
    
    // Receive image size (4 bytes)
    printf("Waiting for size bytes\n");
    uint32_t image_size = 0;
    for (int i = 0; i < 4; i++) {
        int b = getchar_timeout_us(1000000);
        if (b == PICO_ERROR_TIMEOUT) {
            printf("Timeout waiting for size byte %d\n", i);
            return;
        }
        image_size = (image_size << 8) | b;
    }
    printf("Expected image size: %lu bytes\n", image_size);
    putchar('A');  // ACK
    
    // Verify image size
    if (image_size != IMAGE_SIZE) {
        printf("Invalid image size: expected %d, got %lu\n", IMAGE_SIZE, image_size);
        return;
    }
    
    // Set data mode for the entire transfer
    GC9A01_set_data_command(1);
    GC9A01_set_chip_select(0);
    
    printf("Starting data reception\n");
    
    while (total_received < image_size) {
        // Read data in chunks
        int bytes_read = 0;
        while (bytes_read < sizeof(buffer) && total_received + bytes_read < image_size) {
            int c = getchar_timeout_us(1000000);
            if (c == PICO_ERROR_TIMEOUT) {
                printf("Timeout during data receive at byte %d\n", total_received + bytes_read);
                GC9A01_set_chip_select(1);
                return;
            }
            buffer[bytes_read++] = (uint8_t)c;
        }
        
        if (bytes_read > 0) {
            // Send the chunk to display
            GC9A01_spi_tx(buffer, bytes_read);
            total_received += bytes_read;
            putchar('A');  // ACK
            printf("Received chunk: %d bytes (Total: %d/%lu)\n", 
                   bytes_read, total_received, image_size);
        }
    }
    
    GC9A01_set_chip_select(1);
    
    // Wait for END_MARKER
    printf("Waiting for END_MARKER\n");
    int end_marker = getchar_timeout_us(1000000);
    if (end_marker == PICO_ERROR_TIMEOUT || end_marker != 'E') {
        printf("Invalid or missing END_MARKER: 0x%02X\n", end_marker);
        return;
    }
    putchar('A');  // Final ACK
    
    printf("Image receive completed successfully\n");
}

void test_pattern_checkerboard(void) {
    struct GC9A01_frame frame = {{0,0}, {239,239}};
    GC9A01_set_frame(frame);
    
    uint16_t colors[] = {COLOR_BLACK, COLOR_WHITE};
    uint8_t color_bytes[2];
    
    for(int y = 0; y < 240; y++) {
        for(int x = 0; x < 240; x++) {
            uint16_t current_color = colors[((x/20) + (y/20)) % 2];
            color_bytes[0] = current_color >> 8;
            color_bytes[1] = current_color & 0xFF;
            
            if(x == 0 && y == 0) {
                GC9A01_write(color_bytes, 2);
            } else {
                GC9A01_write_continue(color_bytes, 2);
            }
        }
    }
}

void test_pattern_stripes(void) {
    struct GC9A01_frame frame = {{0,0}, {239,239}};
    GC9A01_set_frame(frame);
    
    uint16_t colors[] = {COLOR_RED, COLOR_BLUE};
    uint8_t color_bytes[2];
    
    for(int y = 0; y < 240; y++) {
        uint16_t current_color = colors[(y/30) % 2];
        color_bytes[0] = current_color >> 8;
        color_bytes[1] = current_color & 0xFF;
        
        for(int x = 0; x < 240; x++) {
            if(x == 0 && y == 0) {
                GC9A01_write(color_bytes, 2);
            } else {
                GC9A01_write_continue(color_bytes, 2);
            }
        }
    }
}

void test_pattern_gradient(void) {
    struct GC9A01_frame frame = {{0,0}, {239,239}};
    GC9A01_set_frame(frame);
    
    uint8_t color_bytes[2];
    
    for(int y = 0; y < 240; y++) {
        for(int x = 0; x < 240; x++) {
            uint16_t current_color = RGB565(x & 0xFF, 0, 0);
            color_bytes[0] = current_color >> 8;
            color_bytes[1] = current_color & 0xFF;
            
            if(x == 0 && y == 0) {
                GC9A01_write(color_bytes, 2);
            } else {
                GC9A01_write_continue(color_bytes, 2);
            }
        }
    }
}

// Function to handle display pattern selection
void set_display_pattern(char pattern) {
    switch(pattern) {
        case 'I': // Image
            display_raw_image();
            break;
        case '1':
            test_pattern_checkerboard();
            break;
        case '2':
            test_pattern_stripes();
            break;
        case '3':
            test_pattern_gradient();
            break;
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Wait for USB CDC to be ready
    printf("Display test starting...\n");
    debug_flush();
    
    display_init();
    printf("Display initialized\n");
    
    // Show initial pattern
    test_pattern_checkerboard();
    printf("Initial pattern set\n");
    
    // Show initial pattern
    test_pattern_checkerboard();
    
    // Main loop to handle serial commands
    printf("Entering main loop\n");
    while(1) {
        int c = getchar_timeout_us(100000); // Check every 100ms
        if (c != PICO_ERROR_TIMEOUT) {
            printf("Received command: %c\n", (char)c);
            fflush(stdout);
            set_display_pattern((char)c);
        }
    }
    
    return 0;
}
