#include "GC9A01.h"
#include "deskthang_spi.h"  // Changed from spi.h
#include "hardware/gpio.h"
#include "../common/deskthang_constants.h"
#include <stdio.h>

static uint8_t current_orientation = 0;

void GC9A01_set_orientation(uint8_t orientation) {
    current_orientation = orientation & 0x03;  // Ensure valid range 0-3
}

void GC9A01_write_command(uint8_t cmd) {
    if (!deskthang_spi_is_initialized()) {
        printf("Display Error: SPI not initialized when writing command 0x%02X\n", cmd);
        return;
    }

    GC9A01_set_data_command(0);  // Command mode
    sleep_us(1);                 // Small delay for D/C setup
    
    GC9A01_set_chip_select(0);   // CS active
    sleep_us(1);                 // Small delay for CS setup
    
    bool success = deskthang_spi_write(&cmd, sizeof(cmd));
    
    sleep_us(1);                 // Small delay before CS change
    GC9A01_set_chip_select(1);   // CS inactive
    
    if (!success) {
        printf("Display Error: Failed to write command 0x%02X (SPI error)\n", cmd);
    }
    
    sleep_us(10);  // Delay between commands
}

void GC9A01_write_data(const uint8_t *data, size_t len) {
    if (!deskthang_spi_is_initialized()) {
        printf("Display Error: SPI not initialized when writing %zu bytes of data\n", len);
        return;
    }

    if (!data) {
        printf("Display Error: NULL data pointer\n");
        return;
    }

    GC9A01_set_data_command(1);  // Data mode
    sleep_us(1);                 // Small delay for D/C setup
    
    GC9A01_set_chip_select(0);   // CS active
    sleep_us(1);                 // Small delay for CS setup
    
    bool success = deskthang_spi_write(data, len);
    
    sleep_us(1);                 // Small delay before CS change
    GC9A01_set_chip_select(1);   // CS inactive
    
    if (!success) {
        printf("Display Error: Failed to write %zu bytes of data (SPI error)\n", len);
        // Print first byte if available for debugging
        if (len > 0) {
            printf("Display Error: First byte was 0x%02X\n", data[0]);
        }
    }
    
    sleep_us(10);  // Delay between data writes
}

static inline void GC9A01_write_byte(uint8_t val) {
    GC9A01_write_data(&val, sizeof(val));
    sleep_us(5);  // Small delay between bytes
}

void GC9A01_init(void) {
    printf("Display: Starting initialization sequence\n");
    
    // Check if SPI is initialized
    if (!deskthang_spi_is_initialized()) {
        printf("Display Error: SPI not initialized before display init\n");
        return;
    }
    
    // Check GPIO pins
    printf("Display: Checking GPIO pins...\n");
    if (!gpio_is_dir_out(DISPLAY_PIN_CS) || !gpio_is_dir_out(DISPLAY_PIN_DC) || !gpio_is_dir_out(DISPLAY_PIN_RST)) {
        printf("Display Error: GPIO pins not properly configured\n");
        printf("CS pin (GPIO %d) output: %d\n", DISPLAY_PIN_CS, gpio_is_dir_out(DISPLAY_PIN_CS));
        printf("DC pin (GPIO %d) output: %d\n", DISPLAY_PIN_DC, gpio_is_dir_out(DISPLAY_PIN_DC));
        printf("RST pin (GPIO %d) output: %d\n", DISPLAY_PIN_RST, gpio_is_dir_out(DISPLAY_PIN_RST));
        return;
    }
    printf("Display: GPIO pins configured correctly\n");
    
    GC9A01_set_chip_select(1);
    GC9A01_delay(5);
    printf("Display: Starting reset sequence\n");
    GC9A01_set_reset(0);
    GC9A01_delay(10);
    GC9A01_set_reset(1);
    GC9A01_delay(120);
    printf("Display: Reset sequence complete\n");
    
    /* Initial Sequence */ 
    printf("Display: Starting power control sequence\n");
    GC9A01_write_command(0xEF);
    GC9A01_write_command(0xEB);
    GC9A01_write_byte(0x14);
    
    GC9A01_write_command(0xFE);
    GC9A01_write_command(0xEF);
    
    GC9A01_write_command(0xEB);
    GC9A01_write_byte(0x14);
    
    GC9A01_write_command(0x84);
    GC9A01_write_byte(0x40);
    
    GC9A01_write_command(0x85);
    GC9A01_write_byte(0xFF);
    
    GC9A01_write_command(0x86);
    GC9A01_write_byte(0xFF);
    
    GC9A01_write_command(0x87);
    GC9A01_write_byte(0xFF);
    
    GC9A01_write_command(0x88);
    GC9A01_write_byte(0x0A);
    
    GC9A01_write_command(0x89);
    GC9A01_write_byte(0x21);
    
    GC9A01_write_command(0x8A);
    GC9A01_write_byte(0x00);
    
    GC9A01_write_command(0x8B);
    GC9A01_write_byte(0x80);
    
    GC9A01_write_command(0x8C);
    GC9A01_write_byte(0x01);
    
    GC9A01_write_command(0x8D);
    GC9A01_write_byte(0x01);
    
    GC9A01_write_command(0x8E);
    GC9A01_write_byte(0xFF);
    
    GC9A01_write_command(0x8F);
    GC9A01_write_byte(0xFF);
    printf("Display: Power control sequence complete\n");
    
    printf("Display: Configuring display parameters\n");
    GC9A01_write_command(0xB6);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x00);
    
    printf("Display: Setting orientation (MADCTL)\n");
    GC9A01_write_command(0x36);
#if ORIENTATION == 0
    GC9A01_write_byte(0x18);
#elif ORIENTATION == 1
    GC9A01_write_byte(0x28);
#elif ORIENTATION == 2
    GC9A01_write_byte(0x48);
#else
    GC9A01_write_byte(0x88);
#endif
    
    printf("Display: Setting color mode to 16-bit\n");
    GC9A01_write_command(0x3A);
    GC9A01_write_byte(0x05);
    
    printf("Display: Configuring gamma settings\n");
    GC9A01_write_command(0x90);
    GC9A01_write_byte(0x08);
    GC9A01_write_byte(0x08);
    GC9A01_write_byte(0x08);
    GC9A01_write_byte(0x08);
    
    GC9A01_write_command(0xBD);
    GC9A01_write_byte(0x06);
    
    GC9A01_write_command(0xBC);
    GC9A01_write_byte(0x00);
    
    GC9A01_write_command(0xFF);
    GC9A01_write_byte(0x60);
    GC9A01_write_byte(0x01);
    GC9A01_write_byte(0x04);
    
    printf("Display: Setting power control registers\n");
    GC9A01_write_command(0xC3);
    GC9A01_write_byte(0x13);
    GC9A01_write_command(0xC4);
    GC9A01_write_byte(0x13);
    
    GC9A01_write_command(0xC9);
    GC9A01_write_byte(0x22);
    
    GC9A01_write_command(0xBE);
    GC9A01_write_byte(0x11);
    
    GC9A01_write_command(0xE1);
    GC9A01_write_byte(0x10);
    GC9A01_write_byte(0x0E);
    
    GC9A01_write_command(0xDF);
    GC9A01_write_byte(0x21);
    GC9A01_write_byte(0x0c);
    GC9A01_write_byte(0x02);
    
    GC9A01_write_command(0xF0);
    GC9A01_write_byte(0x45);
    GC9A01_write_byte(0x09);
    GC9A01_write_byte(0x08);
    GC9A01_write_byte(0x08);
    GC9A01_write_byte(0x26);
    GC9A01_write_byte(0x2A);
    
    GC9A01_write_command(0xF1);
    GC9A01_write_byte(0x43);
    GC9A01_write_byte(0x70);
    GC9A01_write_byte(0x72);
    GC9A01_write_byte(0x36);
    GC9A01_write_byte(0x37);
    GC9A01_write_byte(0x6F);
    
    GC9A01_write_command(0xF2);
    GC9A01_write_byte(0x45);
    GC9A01_write_byte(0x09);
    GC9A01_write_byte(0x08);
    GC9A01_write_byte(0x08);
    GC9A01_write_byte(0x26);
    GC9A01_write_byte(0x2A);
    
    GC9A01_write_command(0xF3);
    GC9A01_write_byte(0x43);
    GC9A01_write_byte(0x70);
    GC9A01_write_byte(0x72);
    GC9A01_write_byte(0x36);
    GC9A01_write_byte(0x37);
    GC9A01_write_byte(0x6F);
    
    GC9A01_write_command(0xED);
    GC9A01_write_byte(0x1B);
    GC9A01_write_byte(0x0B);
    
    GC9A01_write_command(0xAE);
    GC9A01_write_byte(0x77);
    
    GC9A01_write_command(0xCD);
    GC9A01_write_byte(0x63);
    
    GC9A01_write_command(0x70);
    GC9A01_write_byte(0x07);
    GC9A01_write_byte(0x07);
    GC9A01_write_byte(0x04);
    GC9A01_write_byte(0x0E);
    GC9A01_write_byte(0x0F);
    GC9A01_write_byte(0x09);
    GC9A01_write_byte(0x07);
    GC9A01_write_byte(0x08);
    GC9A01_write_byte(0x03);
    
    GC9A01_write_command(0xE8);
    GC9A01_write_byte(0x34);
    
    GC9A01_write_command(0x62);
    GC9A01_write_byte(0x18);
    GC9A01_write_byte(0x0D);
    GC9A01_write_byte(0x71);
    GC9A01_write_byte(0xED);
    GC9A01_write_byte(0x70);
    GC9A01_write_byte(0x70);
    GC9A01_write_byte(0x18);
    GC9A01_write_byte(0x0F);
    GC9A01_write_byte(0x71);
    GC9A01_write_byte(0xEF);
    GC9A01_write_byte(0x70);
    GC9A01_write_byte(0x70);
    
    GC9A01_write_command(0x63);
    GC9A01_write_byte(0x18);
    GC9A01_write_byte(0x11);
    GC9A01_write_byte(0x71);
    GC9A01_write_byte(0xF1);
    GC9A01_write_byte(0x70);
    GC9A01_write_byte(0x70);
    GC9A01_write_byte(0x18);
    GC9A01_write_byte(0x13);
    GC9A01_write_byte(0x71);
    GC9A01_write_byte(0xF3);
    GC9A01_write_byte(0x70);
    GC9A01_write_byte(0x70);
    
    GC9A01_write_command(0x64);
    GC9A01_write_byte(0x28);
    GC9A01_write_byte(0x29);
    GC9A01_write_byte(0xF1);
    GC9A01_write_byte(0x01);
    GC9A01_write_byte(0xF1);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x07);
    
    GC9A01_write_command(0x66);
    GC9A01_write_byte(0x3C);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0xCD);
    GC9A01_write_byte(0x67);
    GC9A01_write_byte(0x45);
    GC9A01_write_byte(0x45);
    GC9A01_write_byte(0x10);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x00);
    
    GC9A01_write_command(0x67);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x3C);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x01);
    GC9A01_write_byte(0x54);
    GC9A01_write_byte(0x10);
    GC9A01_write_byte(0x32);
    GC9A01_write_byte(0x98);
    
    GC9A01_write_command(0x74);
    GC9A01_write_byte(0x10);
    GC9A01_write_byte(0x85);
    GC9A01_write_byte(0x80);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x00);
    GC9A01_write_byte(0x4E);
    GC9A01_write_byte(0x00);
    
    GC9A01_write_command(0x98);
    GC9A01_write_byte(0x3e);
    GC9A01_write_byte(0x07);
    
    GC9A01_write_command(0x35);
    GC9A01_write_command(0x21);
    
    printf("Display: Exiting sleep mode\n");
    GC9A01_write_command(0x11);    // Sleep Out
    GC9A01_delay(120);
    
    printf("Display: Turning display on\n");
    GC9A01_write_command(0x29);    // Display ON
    GC9A01_delay(20);
    
    printf("Display: Initialization complete\n");
}

void GC9A01_set_frame(struct GC9A01_frame frame) {

    uint8_t data[4];
    
    GC9A01_write_command(GC9A01_COL_ADDR_SET);
    data[0] = (frame.start.X >> 8) & 0xFF;
    data[1] = frame.start.X & 0xFF;
    data[2] = (frame.end.X >> 8) & 0xFF;
    data[3] = frame.end.X & 0xFF;
    GC9A01_write_data(data, sizeof(data));

    GC9A01_write_command(GC9A01_ROW_ADDR_SET);
    data[0] = (frame.start.Y >> 8) & 0xFF;
    data[1] = frame.start.Y & 0xFF;
    data[2] = (frame.end.Y >> 8) & 0xFF;
    data[3] = frame.end.Y & 0xFF;
    GC9A01_write_data(data, sizeof(data));
    
}

void GC9A01_write(const uint8_t *data, size_t len) {
    GC9A01_set_data_command(1);
    GC9A01_set_chip_select(0);
    deskthang_spi_write(data, len);
    GC9A01_set_chip_select(1);
}

void GC9A01_write_continue(const uint8_t *data, size_t len) {
    GC9A01_write_command(GC9A01_MEM_WR_CONT);
    GC9A01_write_data(data, len);
}

void GC9A01_delay(uint16_t ms) {
    deskthang_delay_ms(ms);
}

void GC9A01_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    struct GC9A01_frame frame = {
        .start = {x, y},
        .end = {x, y}
    };
    GC9A01_set_frame(frame);
    GC9A01_write_command(GC9A01_MEM_WR);
    GC9A01_write_data((uint8_t*)&color, 2);
}

void GC9A01_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    struct GC9A01_frame frame = {
        .start = {x, y},
        .end = {x + w - 1, y + h - 1}
    };
    GC9A01_set_frame(frame);

    // Start memory write
    GC9A01_write_command(GC9A01_MEM_WR);
    
    // Calculate total number of pixels
    uint32_t total_pixels = w * h;
    
    // Write first pixel
    GC9A01_write_data((uint8_t*)&color, 2);
    
    // Write remaining pixels using continue command
    for(uint32_t i = 1; i < total_pixels; i++) {
        GC9A01_write_continue((uint8_t*)&color, 2);
    }
}

uint8_t GC9A01_read_status(void) {
    return 0;  // Placeholder for compatibility
}

uint8_t GC9A01_read_display_mode(void) {
    return 0;  // Placeholder for compatibility
}

uint8_t GC9A01_read_memory_access(void) {
    return 0;  // Placeholder for compatibility
}
