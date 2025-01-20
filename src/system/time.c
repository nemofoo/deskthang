#include "time.h"
#include "pico/stdlib.h"

uint32_t get_system_time(void) {
    return to_ms_since_boot(get_absolute_time());
}

void delay_ms(uint32_t ms) {
    sleep_ms(ms);
}

void delay_us(uint32_t us) {
    sleep_us(us);
}
