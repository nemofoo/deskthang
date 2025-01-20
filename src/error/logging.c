#include "logging.h"
#include "../system/time.h"
#include "../hardware/serial.h"
#include "../common/deskthang_constants.h"
#include <stdio.h>

// Static configuration
static bool logging_enabled = false;

// Internal helper functions
static void send_message(const char *prefix, const char *message) {
    if (!message) {
        return;
    }

    char buffer[MESSAGE_BUFFER_SIZE];
    int len = snprintf(buffer, sizeof(buffer),
                "%s [%lu] %s",
                prefix,
                deskthang_time_get_ms(),
                message);

    if (len > 0 && len < sizeof(buffer)) {
        serial_write((uint8_t*)buffer, len);
        serial_write((uint8_t*)"\n", 1);  // Add newline for readability
    }
}

// Initialize logging
bool logging_init(void) {
    logging_enabled = true;
    return true;
}

// Basic logging
void logging_write(const char *module, const char *message) {
    if (!logging_enabled || !module || !message) return;
    send_message(module, message);
}

// Error logging with details
void logging_error_details(const ErrorDetails *error) {
    if (!logging_enabled || !error) return;

    char message[256];
    snprintf(message, sizeof(message), "%s (Code: %u)", 
             error->message, error->code);

    send_message("ERROR", message);
}
