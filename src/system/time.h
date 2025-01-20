#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <stdint.h>

// Get system time in milliseconds
uint32_t get_system_time(void);

// Delay functions
void delay_ms(uint32_t ms);
void delay_us(uint32_t us);

#endif // SYSTEM_TIME_H
