#ifndef __GC9A01_H
#define __GC9A01_H

#include <stdint.h>
#include <stddef.h>
#include "../common/deskthang_constants.h"
#include "deskthang_gpio.h"  // Update if it's using gpio.h
#include "../system/time.h"

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
void GC9A01_set_frame(struct GC9A01_frame frame);
void GC9A01_write(const uint8_t *data, size_t len);
void GC9A01_write_continue(const uint8_t *data, size_t len);
void GC9A01_write_data(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif
