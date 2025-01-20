#include "logging.h"
#include <string.h>
#include "../system/time.h"
#include <stdio.h>
#include "../hardware/serial.h"
#include "../protocol/packet.h"
#include "../protocol/protocol.h"

// Debug packet format
#define DEBUG_PACKET_PREFIX "[LOG]"
#define DEBUG_PACKET_MAX_SIZE MAX_PACKET_SIZE

// Static configuration
static LogConfig log_config = {0};

static bool send_debug_packet(const char *module, const char *message, const char *context) {
    if (!log_config.enabled) {
        return false;
    }

    // Create packet
    Packet packet = {0};
    packet.header.type = PACKET_DEBUG;
    packet.header.sequence = 0;  // Debug packets don't use sequence numbers

    // Format debug message
    char debug_message[DEBUG_PACKET_MAX_SIZE];
    if (context) {
        snprintf(debug_message, sizeof(debug_message), 
                "%s [%u] [%s] %s | %s",
                DEBUG_PACKET_PREFIX, time_get_ms(), module, message, context);
    } else {
        snprintf(debug_message, sizeof(debug_message),
                "%s [%u] [%s] %s",
                DEBUG_PACKET_PREFIX, time_get_ms(), module, message);
    }

    // Copy message to packet payload
    size_t msg_len = strlen(debug_message) + 1;  // Include null terminator
    memcpy(packet.payload, debug_message, msg_len);  // Changed from data to payload
    packet.header.length = msg_len;

    // Send packet
    return serial_write((uint8_t*)&packet, sizeof(PacketHeader) + packet.header.length);
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
