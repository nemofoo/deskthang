#include "logging.h"
#include <time.h>
#include "../hardware/serial.h"
#include "../protocol/packet.h"
#include "../common/deskthang_constants.h"
#include <stdio.h>
#include <stdlib.h>  // For free()

// Static configuration
static bool logging_enabled = true;
static bool use_debug_packets = true;

// Internal helper functions
static bool send_message(const char *module, const char *message) {
    if (!logging_enabled) {
        return false;
    }
    
    if (use_debug_packets) {
        // Create and send debug packet
        Packet packet;
        if (!packet_create_debug(&packet, module, message)) {
            return false;
        }
        
        bool result = packet_transmit(&packet);
        packet_free(&packet);
        return result;
    } else {
        // Fallback to direct serial write
        char line[MAX_LINE_SIZE];
        int len = snprintf(line, sizeof(line), "[%s] %s\n", module, message);
        if (len < 0 || len >= sizeof(line)) {
            return false;
        }
        return serial_write((uint8_t*)line, len);
    }
}

// Initialize logging
bool logging_init(void) {
    logging_enabled = true;
    use_debug_packets = true;  // Start with debug packets
    return true;
}

// Enable debug packet mode
void logging_enable_debug_packets(void) {
    use_debug_packets = true;
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
