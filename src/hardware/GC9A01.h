#ifndef __GC9A01_H
#define __GC9A01_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Command codes:
#define COL_ADDR_SET        0x2A
#define ROW_ADDR_SET        0x2B
#define MEM_WR              0x2C
#define COLOR_MODE          0x3A
#define COLOR_MODE__12_BIT  0x03
#define COLOR_MODE__16_BIT  0x05
#define COLOR_MODE__18_BIT  0x06
#define MEM_WR_CONT         0x3C
#define GC9A01_COL_ADDR_SET        0x2A
#define GC9A01_ROW_ADDR_SET        0x2B
#define GC9A01_MEM_WR              0x2C
#define GC9A01_MEM_WR_CONT         0x3C
#define GC9A01_COLOR_MODE          0x3A
#define GC9A01_COLOR_MODE__12_BIT  0x03
#define GC9A01_COLOR_MODE__16_BIT  0x05
#define GC9A01_COLOR_MODE__18_BIT  0x06

// Orientation
#define ORIENTATION_0   0x18
#define ORIENTATION_90  0x28
#define ORIENTATION_180 0x48
#define ORIENTATION_270 0x88

// Hardware abstraction layer
// Should be defined by the user of the library
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
void GC9A01_set_frame(struct GC9A01_frame frame);
void GC9A01_write(uint8_t *data, size_t len);
void GC9A01_write_continue(uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
