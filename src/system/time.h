#ifndef SYSTEM_TIME_H
#define SYSTEM_TIME_H

#include <stdint.h>

/**
 * Get current system time in milliseconds
 * @return Current time in milliseconds
 */
uint32_t deskthang_time_get_ms(void);

/**
 * Delay execution for specified milliseconds
 * @param ms Number of milliseconds to delay
 */
void deskthang_delay_ms(uint32_t ms);

/**
 * Delay execution for specified microseconds
 * @param us Number of microseconds to delay
 */
void deskthang_delay_us(uint32_t us);

#endif // SYSTEM_TIME_H
