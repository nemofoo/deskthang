#include "logging.h"
#include <string.h>
#include <stdio.h>
#include "../hardware/serial.h"
#include "../protocol/packet.h"
#include "../protocol/protocol.h"

// Debug packet format
#define DEBUG_PACKET_PREFIX "[LOG]"
#define DEBUG_PACKET_MAX_SIZE MAX_PACKET_SIZE

// Default log configuration
static LogConfig g_log_config = {
    .enabled = true
};

// Format and send debug packet
static void send_debug_packet(const char *module, 
                            const char *message, 
                            const char *context) {
    // Format debug message
    char debug_buffer[DEBUG_PACKET_MAX_SIZE];
    int written = snprintf(debug_buffer, sizeof(debug_buffer),
        "%s [%u] %s (%s)%s%s\n",
        DEBUG_PACKET_PREFIX,
        get_system_time(),
        message ? message : "",
        module ? module : "unknown",
        context ? " | " : "",
        context ? context : "");

    if (written > 0 && written < DEBUG_PACKET_MAX_SIZE) {
        // Create debug packet
        Packet packet;
        if (packet_create_debug(&packet, module, debug_buffer)) {
            // Send via serial with protocol-defined timeout
            serial_write_exact((uint8_t*)&packet, sizeof(PacketHeader) + packet.header.length);
            serial_flush();
        }
    }
}

bool logging_init(void) {
    // Initialize serial debug if enabled
    if (g_log_config.enabled) {
        if (!serial_init(0)) {  // Baud rate ignored for USB CDC
            g_log_config.enabled = false;  // Disable if init fails
            return false;
        }
    }
    return true;
}

void logging_write(const char *module, const char *message) {
    logging_write_with_context(module, message, NULL);
}

void logging_write_with_context(const char *module, 
                              const char *message, 
                              const char *context) {
    // Send to USB serial if enabled
    if (g_log_config.enabled) {
        send_debug_packet(module, message, context);
    }
}

void logging_error(const ErrorDetails *error) {
    if (!error || !g_log_config.enabled) {
        return;
    }
    
    char context[512];
    snprintf(context, sizeof(context),
             "Category: %s, Severity: %s, Code: %u, Recoverable: %s",
             error_category_to_string(error->category),
             error_severity_to_string(error->severity),
             error->code,
             error->recoverable ? "Yes" : "No");
             
    logging_write_with_context("Error",
                              error->message,
                              context);
}

void logging_recovery(const RecoveryResult *result) {
    if (!result || !g_log_config.enabled) {
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

void logging_write_structured(
    const char *module,
    const char *message,
    const char *context,
    LogLevel level,
    uint32_t code
) {
    // Implementation needed to match documentation
}
