#include "serial.h"
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "tusb.h"
#include "../error/logging.h"

#define SERIAL_BUFFER_SIZE MAX_PACKET_SIZE
#define POLL_INTERVAL_MS 1

static uint8_t rx_buffer[SERIAL_BUFFER_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
static bool is_initialized = false;

bool serial_init(uint32_t baud_rate) {
    if (is_initialized) {
        return true;
    }

    // Initialize USB serial
    stdio_init_all();
    
    // Wait for USB CDC to be ready
    while (!tud_cdc_connected()) {
        sleep_ms(100);
    }

    is_initialized = true;
    log_info("Serial", "USB CDC initialized");
    return true;
}

bool serial_read_exact(uint8_t *buffer, uint16_t size, uint32_t timeout_ms) {
    if (!buffer || size == 0) {
        return false;
    }

    absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);
    uint16_t bytes_read = 0;

    while (bytes_read < size) {
        if (absolute_time_diff_us(get_absolute_time(), timeout_time) <= 0) {
            log_error("Serial", "Read timeout after %d bytes", bytes_read);
            return false;
        }

        // Check for available data
        if (!tud_cdc_available()) {
            sleep_ms(POLL_INTERVAL_MS);
            continue;
        }

        // Read data
        uint32_t count = tud_cdc_read(buffer + bytes_read, size - bytes_read);
        if (count > 0) {
            bytes_read += count;
        }
    }

    return true;
}

int16_t serial_read(uint8_t *buffer, uint16_t size, uint32_t timeout_ms) {
    if (!buffer || size == 0) {
        return -1;
    }

    absolute_time_t timeout_time = make_timeout_time_ms(timeout_ms);
    uint16_t bytes_read = 0;

    while (bytes_read < size) {
        if (absolute_time_diff_us(get_absolute_time(), timeout_time) <= 0) {
            break;  // Return partial read on timeout
        }

        if (!tud_cdc_available()) {
            sleep_ms(POLL_INTERVAL_MS);
            continue;
        }

        uint32_t count = tud_cdc_read(buffer + bytes_read, size - bytes_read);
        if (count > 0) {
            bytes_read += count;
        }
    }

    return bytes_read > 0 ? bytes_read : -1;
}

bool serial_write_exact(const uint8_t *buffer, uint16_t size) {
    if (!buffer || size == 0) {
        return false;
    }

    uint16_t bytes_written = 0;
    while (bytes_written < size) {
        uint32_t count = tud_cdc_write(buffer + bytes_written, size - bytes_written);
        if (count > 0) {
            bytes_written += count;
            // Flush after each chunk to ensure data is sent
            if (bytes_written % CHUNK_SIZE == 0) {
                serial_flush();
            }
        } else {
            sleep_ms(POLL_INTERVAL_MS);
        }
    }

    // Final flush
    serial_flush();
    return true;
}

int16_t serial_write(const uint8_t *buffer, uint16_t size) {
    if (!buffer || size == 0) {
        return -1;
    }

    uint16_t bytes_written = 0;
    while (bytes_written < size) {
        uint32_t count = tud_cdc_write(buffer + bytes_written, size - bytes_written);
        if (count > 0) {
            bytes_written += count;
        } else {
            break;  // Return partial write if buffer is full
        }
    }

    return bytes_written > 0 ? bytes_written : -1;
}

void serial_flush(void) {
    tud_cdc_write_flush();
}

bool serial_available(void) {
    return tud_cdc_available() > 0;
}

void serial_clear(void) {
    uint8_t dummy[64];
    while (tud_cdc_available()) {
        tud_cdc_read(dummy, sizeof(dummy));
    }
}

void serial_deinit(void) {
    if (is_initialized) {
        is_initialized = false;
        log_info("Serial", "USB CDC deinitialized");
    }
}

bool serial_write_debug(const char *module, const char *message) {
    DebugPacket packet;
    packet.timestamp = get_system_time();
    strncpy(packet.module, module, sizeof(packet.module)-1);
    strncpy(packet.message, message, sizeof(packet.message)-1);
    
    return serial_write_exact((uint8_t*)&packet, sizeof(packet));
}

bool serial_write_chunked(const uint8_t *data, size_t len) {
    size_t remaining = len;
    while (remaining > 0) {
        size_t chunk = min(remaining, CHUNK_SIZE);
        if (!serial_write_exact(data, chunk)) {
            return false;
        }
        data += chunk;
        remaining -= chunk;
    }
    return true;
}
