#include "logging.h"
#include "../system/time.h"
#include "../hardware/serial.h"
#include <stdio.h>
#include <stdarg.h>

// Debug packet format
#define DEBUG_PACKET_PREFIX "[LOG]"
#define DEBUG_PACKET_MAX_SIZE MAX_PACKET_SIZE

// Static configuration
static LogConfig log_config = {0};

void send_debug_packet(const char *module, const char *message, const char *context) {
    if (!module || !message) {
        return;
    }

    char buffer[MAX_PACKET_SIZE];
    int len = snprintf(buffer, sizeof(buffer),
                "%s [%lu] %s: %s %s",
                DEBUG_PACKET_PREFIX, 
                deskthang_time_get_ms(), 
                module, 
                message, 
                context ? context : "");

    if (len > 0 && len < sizeof(buffer)) {
        serial_write((uint8_t*)buffer, len);
    }
}

bool logging_init(void) {
    if (!serial_init()) {  // Remove baud rate parameter
        return false;
    }
    log_config.enabled = true;
    return true;
}

void logging_write(const char *module, const char *message) {
    send_debug_packet(module, message, NULL);
}

void logging_write_with_context(const char *module, const char *message, const char *context) {
    send_debug_packet(module, message, context);
}

void logging_error(const ErrorDetails *error) {
    if (!error) {
        return;
    }

    char message[512];
    snprintf(message, sizeof(message),
             "%s (ERR!) | Type: %s, Severity: %s, Code: %u, Recoverable: %s",
             error->message,
             error_type_to_string(error->type),
             error_severity_to_string(error->severity),
             error->code,
             error->recoverable ? "yes" : "no");

    // Use NULL for module since timestamp is handled in send_debug_packet
    logging_write("Error", message);
}

void logging_recovery(const RecoveryResult *result) {
    if (!result) {
        return;
    }
    
    char context[512];
    snprintf(context, sizeof(context),
             "Duration: %ums, Attempts: %u",
             result->duration_ms,
             result->attempts);
             
    logging_write_with_context("Recovery",
                              result->message,
                              context);
}

void log_info(const char* message) {
    if (!message) {
        return;
    }
    send_debug_packet("INFO", message, NULL);
}
