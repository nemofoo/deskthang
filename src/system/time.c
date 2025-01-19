#include "time.h"
#include "pico/stdlib.h"

uint32_t get_system_time(void) {
    return to_ms_since_boot(get_absolute_time());
}
