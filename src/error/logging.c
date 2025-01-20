#include "logging.h"
#include "../system/time.h"
#include "../hardware/serial.h"
#include "../common/deskthang_constants.h"
#include <stdio.h>
#include <stdarg.h>

// Static configuration
static LogConfig log_config = {0};

// Internal helper functions
static void send_message(const char *prefix, const char *module, const char *message, const char *context) {
    if (!module || !message) {
        return;
    }

    char buffer[MESSAGE_BUFFER_SIZE];
    int len = snprintf(buffer, sizeof(buffer),
                MESSAGE_FORMAT_LOG,
                prefix,
                deskthang_time_get_ms(),
                module,
                message,
                context ? " " : "",
                context ? context : "");

    if (len > 0 && len < sizeof(buffer)) {
        serial_write((uint8_t*)buffer, len);
        serial_write((uint8_t*)"\n", 1);  // Add newline for readability
    }
}

static void send_error_message(const char *module, uint32_t code, const char *message, const char *context) {
    if (!module || !message) {
        return;
    }

    char buffer[MESSAGE_BUFFER_SIZE];
    int len = snprintf(buffer, sizeof(buffer),
                MESSAGE_FORMAT_ERROR,
                MESSAGE_PREFIX_ERROR,
                deskthang_time_get_ms(),
                module,
                code,
                message,
                context ? " " : "",
                context ? context : "");

    if (len > 0 && len < sizeof(buffer)) {
        serial_write((uint8_t*)buffer, len);
        serial_write((uint8_t*)"\n", 1);  // Add newline for readability
    }
}

bool logging_init(void) {
    if (!serial_init()) {
        return false;
    }
    log_config.enabled = true;
    return true;
}

void logging_write(const char *module, const char *message) {
    if (!log_config.enabled) return;
    send_message(MESSAGE_PREFIX_LOG, module, message, NULL);
}

void logging_write_with_context(const char *module, const char *message, const char *context) {
    if (!log_config.enabled) return;
    send_message(MESSAGE_PREFIX_LOG, module, message, context);
}

void logging_error(const ErrorDetails *error) {
    if (!log_config.enabled || !error) return;
    
    // Get string representations
    const char *module = error_type_to_string(error->type);
    
    // Send error message with code and context
    send_error_message(
        module,
        error->code,
        error->message,
        error->context[0] != '\0' ? error->context : NULL
    );
}

void logging_error_with_context(const char *module, uint32_t code, const char *message, const char *context) {
    if (!log_config.enabled) return;
    send_error_message(module, code, message, context);
}

void logging_set_enabled(bool enabled) {
    log_config.enabled = enabled;
}

bool logging_is_enabled(void) {
    return log_config.enabled;
}
