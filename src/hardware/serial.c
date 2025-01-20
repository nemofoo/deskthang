#include "../system/time.h"
#include "serial.h"
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include <stdio.h>      // For fwrite, fflush, stdout
#include "pico/binary_info/code.h"  // For string operations
#include "../error/logging.h"
#include <string.h>     // For strncpy
#include "../debug/debug.h"

// Static configuration
static struct {
    bool initialized;
    uint32_t timeout_ms;
    uint32_t overflow_count;
    uint32_t last_overflow_time;
    bool in_overflow;
} serial_state = {
    .initialized = false,
    .timeout_ms = BASE_TIMEOUT_MS,
    .overflow_count = 0,
    .last_overflow_time = 0,
    .in_overflow = false
};

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
    if (!serial_state.initialized) {
        return false;
    }

    // Check for potential overflow
    if (len > CHUNK_SIZE && !serial_state.in_overflow) {
        serial_state.in_overflow = true;
        serial_state.overflow_count++;
        serial_state.last_overflow_time = deskthang_time_get_ms();
        
        debug_log_overflow();
        
        ErrorDetails error = {
            .type = ERROR_TYPE_HARDWARE,
            .severity = ERROR_SEVERITY_ERROR,
            .code = ERROR_SERIAL_OVERFLOW,
            .timestamp = deskthang_time_get_ms(),
            .source_state = 0,
            .recoverable = true,
            .retry_count = 0,
            .backoff_ms = 0
        };
        strncpy(error.message, "Buffer overflow detected", ERROR_MESSAGE_SIZE - 1);
        snprintf(error.context, ERROR_CONTEXT_SIZE - 1, "Buffer size: %d, Write size: %zu", CHUNK_SIZE, len);
        logging_error_details(&error);
        
        // Attempt recovery by flushing
        serial_flush();
        serial_clear();
        
        // Add delay to allow system to recover
        sleep_ms(10);
    }

    debug_log_buffer_usage(len, CHUNK_SIZE);
    debug_log_operation_start("serial_write");

    size_t remaining = len;
    const uint8_t *ptr = data;
    uint32_t retry_count = 0;
    const uint32_t max_retries = 3;
    
    while (remaining > 0 && retry_count < max_retries) {
        size_t chunk = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
        if (!serial_write_chunk(ptr, chunk)) {
            retry_count++;
            if (retry_count < max_retries) {
                // Log retry attempt
                char msg[64];
                snprintf(msg, sizeof(msg), "Retrying write (attempt %u/%u)", retry_count + 1, max_retries);
                logging_write("Serial", msg);
                
                // Flush and wait before retry
                serial_flush();
                sleep_ms(5 * retry_count);  // Exponential backoff
                continue;
            }
            
            debug_log_operation_end("serial_write");
            return false;
        }
        ptr += chunk;
        remaining -= chunk;
        retry_count = 0;  // Reset retry count on successful write
    }

    // Clear overflow state if write succeeds
    if (serial_state.in_overflow) {
        serial_state.in_overflow = false;
        logging_write("Serial", "Recovered from overflow condition");
    }

    debug_log_operation_end("serial_write");
    return remaining == 0;  // Return true only if all data was written
}

bool serial_write_chunk(const uint8_t *data, size_t len) {
    if (!serial_state.initialized || len > CHUNK_SIZE) {
        return false;
    }

    debug_log_operation_start("serial_write_chunk");

    // Try to write with timeout
    absolute_time_t timeout = make_timeout_time_ms(SERIAL_WRITE_TIMEOUT_MS);
    while (len > 0) {
        if (time_reached(timeout)) {
            debug_log_operation_end("serial_write_chunk");
            
            ErrorDetails error = {
                .type = ERROR_TYPE_HARDWARE,
                .severity = ERROR_SEVERITY_ERROR,
                .code = ERROR_SERIAL_TIMEOUT,
                .timestamp = deskthang_time_get_ms(),
                .source_state = 0,
                .recoverable = true,
                .retry_count = 0,
                .backoff_ms = 0
            };
            strncpy(error.message, "Write timeout", ERROR_MESSAGE_SIZE - 1);
            error.message[ERROR_MESSAGE_SIZE - 1] = '\0';
            error.context[0] = '\0';
            logging_error_details(&error);
            return false;
        }

        int written = fwrite(data, 1, len, stdout);
        if (written <= 0) {
            debug_log_operation_end("serial_write_chunk");
            return false;
        }
        
        data += written;
        len -= written;
    }

    debug_log_operation_end("serial_write_chunk");
    return true;
}

bool serial_read(uint8_t *data, size_t len) {
    if (!serial_state.initialized || data == NULL) {
        return false;
    }

    uint32_t start = deskthang_time_get_ms();
    size_t total_read = 0;

    while (total_read < len) {
        if (deskthang_time_get_ms() - start > serial_state.timeout_ms) {
            // Only report timeout as error if we've started reading data
            if (total_read > 0) {
                ErrorDetails error = {
                    .type = ERROR_TYPE_HARDWARE,
                    .severity = ERROR_SEVERITY_WARNING,
                    .code = ERROR_SERIAL_UNDERFLOW,
                    .timestamp = deskthang_time_get_ms(),
                    .source_state = 0,
                    .recoverable = true,
                    .retry_count = 0,
                    .backoff_ms = 0
                };
                strncpy(error.message, "Incomplete read detected", ERROR_MESSAGE_SIZE - 1);
                error.message[ERROR_MESSAGE_SIZE - 1] = '\0';
                snprintf(error.context, ERROR_CONTEXT_SIZE - 1, "Expected %zu bytes, got %zu", len, total_read);
                logging_error_details(&error);
            }
            return false;  // Timeout
        }

        int c = getchar_timeout_us(0);  // Non-blocking read
        if (c == PICO_ERROR_TIMEOUT) {
            // No data available, not an error
            return false;
        }
        
        if (c != PICO_ERROR_TIMEOUT) {
            data[total_read++] = (uint8_t)c;
        }
    }

    return total_read == len;
}

bool serial_write_debug(const char *module, const char *message) {
    if (!serial_state.initialized || !module || !message) {
        return false;
    }

    char buffer[MESSAGE_BUFFER_SIZE];
    int len = snprintf(buffer, sizeof(buffer),
                MESSAGE_FORMAT_LOG,
                MESSAGE_PREFIX_LOG,
                deskthang_time_get_ms(),
                module,
                message,
                "", "");  // No context for debug messages

    if (len > 0 && len < sizeof(buffer)) {
        bool success = serial_write((uint8_t*)buffer, len);
        if (success) {
            return serial_write((uint8_t*)"\n", 1);  // Add newline
        }
    }
    return false;
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
        // Reset overflow state after successful flush
        if (serial_state.in_overflow) {
            serial_state.in_overflow = false;
            logging_write("Serial", "Overflow cleared after flush");
        }
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

bool serial_get_stats(SerialStats *stats) {
    if (!stats) {
        return false;
    }
    
    stats->overflow_count = serial_state.overflow_count;
    stats->last_overflow_time = serial_state.last_overflow_time;
    stats->in_overflow = serial_state.in_overflow;

    // Also update debug stats
    ResourceDebugStats *debug_stats = debug_get_resource_stats();
    debug_stats->total_overflows = serial_state.overflow_count;
    debug_stats->last_overflow_time = serial_state.last_overflow_time;
    
    return true;
}
