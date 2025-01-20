#include "serial.h"
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include <stdio.h>      // For fwrite, fflush, stdout
#include "pico/binary_info/code.h"  // For string operations
#include "../system/time.h"
#include "../error/logging.h"
#include <string.h>     // For strncpy

// Static configuration
static struct {
    bool initialized;
    uint32_t timeout_ms;
} serial_state = {0};

bool serial_init(void) {
    if (serial_state.initialized) {
        return true;
    }

    // Initialize USB stdio
    stdio_init_all();
    serial_state.timeout_ms = BASE_TIMEOUT_MS;
    serial_state.initialized = true;

    return true;
}

void serial_deinit(void) {
    serial_state.initialized = false;
}

bool serial_write(const uint8_t *data, size_t len) {
    if (!serial_state.initialized || data == NULL) {
        return false;
    }

    size_t written = fwrite(data, 1, len, stdout);
    fflush(stdout);  // Ensure data is sent
    return written == len;
}

bool serial_read(uint8_t *data, size_t len) {
    if (!serial_state.initialized || data == NULL) {
        return false;
    }

    uint32_t start = get_system_time();
    size_t total_read = 0;

    while (total_read < len) {
        if (get_system_time() - start > serial_state.timeout_ms) {
            return false;  // Timeout
        }

        int c = getchar_timeout_us(0);  // Non-blocking read
        if (c != PICO_ERROR_TIMEOUT) {
            data[total_read++] = (uint8_t)c;
        }
    }

    return true;
}

bool serial_write_debug(const char *module, const char *message) {
    if (!serial_state.initialized) {
        return false;
    }

    DebugPacket packet;
    packet.timestamp = get_system_time();
    strncpy(packet.module, module, sizeof(packet.module)-1);
    packet.module[sizeof(packet.module)-1] = '\0';
    strncpy(packet.message, message, sizeof(packet.message)-1);
    packet.message[sizeof(packet.message)-1] = '\0';
    
    return serial_write((uint8_t*)&packet, sizeof(packet));
}

bool serial_write_chunked(const uint8_t *data, size_t len) {
    if (!serial_state.initialized || data == NULL) {
        return false;
    }

    size_t remaining = len;
    const uint8_t *ptr = data;
    
    while (remaining > 0) {
        size_t chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        if (!serial_write(ptr, chunk)) {
            return false;
        }
        ptr += chunk;
        remaining -= chunk;
    }
    return true;
}

void serial_flush(void) {
    if (serial_state.initialized) {
        fflush(stdout);
    }
}

bool serial_available(void) {
    return serial_state.initialized && (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT);
}

void serial_clear(void) {
    if (!serial_state.initialized) {
        return;
    }
    
    while (getchar_timeout_us(0) != PICO_ERROR_TIMEOUT) {
        // Keep reading until no more data
    }
}
