#include "time.h"
#include "pico/stdlib.h"

uint32_t deskthang_time_get_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

void deskthang_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

void deskthang_delay_us(uint32_t us) {
    sleep_us(us);
}
