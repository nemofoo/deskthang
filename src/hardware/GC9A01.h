#ifndef __GC9A01_H
#define __GC9A01_H

#include <stdint.h>
#include <stddef.h>
#include "../common/deskthang_constants.h"
#include "deskthang_gpio.h"  // Update if it's using gpio.h
#include "../system/time.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Hardware abstraction layer
void GC9A01_set_reset(uint8_t val);
void GC9A01_set_data_command(uint8_t val);
void GC9A01_set_chip_select(uint8_t val);
void GC9A01_delay(uint16_t ms);
void GC9A01_spi_tx(uint8_t *data, size_t len);

// Helper function to write a command
void GC9A01_write_command(uint8_t cmd);

struct GC9A01_point {
    uint16_t X, Y;
};

struct GC9A01_frame {
    struct GC9A01_point start, end;
};

void GC9A01_init(void);
void GC9A01_set_orientation(uint8_t orientation);
void GC9A01_set_frame(struct GC9A01_frame frame);
void GC9A01_write(const uint8_t *data, size_t len);
void GC9A01_write_continue(const uint8_t *data, size_t len);
void GC9A01_write_data(const uint8_t *data, size_t len);

// Drawing functions
void GC9A01_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void GC9A01_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

// Status register bits
#define GC9A01_STATUS_READY      0x80
#define GC9A01_STATUS_BUSY       0x40
#define GC9A01_STATUS_ERROR      0x20

// Display mode masks
#define GC9A01_MODE_VALID_MASK   0x0F
#define GC9A01_MODE_EXPECTED     0x04

// Memory access masks
#define GC9A01_MEM_ACCESS_MASK   0x1C
#define GC9A01_MEM_ACCESS_EXPECTED 0x0C

// Function declarations
uint8_t GC9A01_read_status(void);
uint8_t GC9A01_read_display_mode(void);
uint8_t GC9A01_read_memory_access(void);

#ifdef __cplusplus
}
#endif

#endif
