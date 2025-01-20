#include "time.h"
#include "pico/stdlib.h"

static bool time_initialized = false;

uint32_t deskthang_time_get_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}

void deskthang_delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

void deskthang_delay_us(uint32_t us) {
    sleep_us(us);
}

bool deskthang_time_is_initialized(void) {
    return time_initialized;
}
