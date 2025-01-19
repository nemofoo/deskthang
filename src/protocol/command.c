#include "command.h"
#include <string.h>
#include <stdlib.h>

// Global command context
static CommandContext g_command_context;
static CommandStatus g_command_status;

// Initialize command processing
bool command_init(void) {
    memset(&g_command_context, 0, sizeof(CommandContext));
    memset(&g_command_status, 0, sizeof(CommandStatus));
    return true;
}

// Reset command state
void command_reset(void) {
    if (g_command_context.command_data) {
        free(g_command_context.command_data);
    }
    memset(&g_command_context, 0, sizeof(CommandContext));
    memset(&g_command_status, 0, sizeof(CommandStatus));
}

// Get command context
CommandContext *command_get_context(void) {
    return &g_command_context;
}

// Process incoming command packet
bool command_process(const Packet *packet) {
    if (!packet || packet_get_type(packet) != PACKET_CMD) {
        return false;
    }
    
    const uint8_t *payload = packet_get_payload(packet);
    if (!payload || packet_get_length(packet) < 1) {
        return false;
    }
    
    CommandType cmd = (CommandType)payload[0];
    if (!command_validate_type(cmd)) {
        return false;
    }
    
    // Handle command based on type
    bool success = false;
    switch (cmd) {
        case CMD_IMAGE_START:
            success = command_start_image_transfer();
            break;
            
        case CMD_IMAGE_END:
            success = command_end_image_transfer();
            break;
            
        case CMD_PATTERN_CHECKER:
            success = command_show_checkerboard();
            break;
            
        case CMD_PATTERN_STRIPE:
            success = command_show_stripes();
            break;
            
        case CMD_PATTERN_GRADIENT:
            success = command_show_gradient();
            break;
            
        case CMD_HELP:
            success = command_show_help();
            break;
    }
    
    if (success) {
        g_command_context.type = cmd;
        g_command_context.start_time = get_system_time();
        g_command_context.in_progress = true;
    }
    
    return success;
}

// Complete current command
bool command_complete(void) {
    if (!g_command_context.in_progress) {
        return false;
    }
    
    // Update status
    g_command_status.duration_ms = get_system_time() - g_command_context.start_time;
    g_command_status.bytes_processed = g_command_context.bytes_processed;
    g_command_status.success = true;
    
    // Reset context
    command_reset();
    
    return true;
}

// Abort current command
bool command_abort(void) {
    if (!g_command_context.in_progress) {
        return false;
    }
    
    // Update status
    g_command_status.success = false;
    strncpy(g_command_status.message, "Command aborted", sizeof(g_command_status.message) - 1);
    
    // Reset context
    command_reset();
    
    return true;
}

// Command validation
bool command_validate_type(CommandType type) {
    switch (type) {
        case CMD_IMAGE_START:
        case CMD_IMAGE_END:
        case CMD_PATTERN_CHECKER:
        case CMD_PATTERN_STRIPE:
        case CMD_PATTERN_GRADIENT:
        case CMD_HELP:
            return true;
        default:
            return false;
    }
}

bool command_validate_state(void) {
    SystemState current = state_machine_get_current();
    return current == STATE_COMMAND_PROCESSING || 
           current == STATE_DATA_TRANSFER;
}

bool command_validate_sequence(const Packet *packet) {
    return packet_validate_sequence(packet_get_sequence(packet));
}

// Image transfer commands
bool command_start_image_transfer(void) {
    if (g_command_context.in_progress) {
        return false;
    }
    
    // Initialize transfer context
    g_command_context.bytes_processed = 0;
    g_command_context.total_bytes = 240 * 240 * 2; // RGB565 format
    
    // Transition to data transfer state
    return state_machine_transition(STATE_DATA_TRANSFER, CONDITION_TRANSFER_START);
}

bool command_process_image_chunk(const uint8_t *data, uint16_t length) {
    if (!g_command_context.in_progress || !data) {
        return false;
    }
    
    // Update progress
    g_command_context.bytes_processed += length;
    
    // TODO: Process image data chunk
    
    return true;
}

bool command_end_image_transfer(void) {
    if (!g_command_context.in_progress) {
        return false;
    }
    
    // Verify all data received
    if (g_command_context.bytes_processed != g_command_context.total_bytes) {
        return false;
    }
    
    // Return to ready state
    return state_machine_transition(STATE_READY, CONDITION_TRANSFER_COMPLETE);
}

// Pattern commands
bool command_show_checkerboard(void) {
    // TODO: Implement checkerboard pattern
    return true;
}

bool command_show_stripes(void) {
    // TODO: Implement stripe pattern
    return true;
}

bool command_show_gradient(void) {
    // TODO: Implement gradient pattern
    return true;
}

// Help command
bool command_show_help(void) {
    const char *help_text =
        "Available commands:\n"
        "I: Start image transfer (RGB565 format, 240×240)\n"
        "E: End image transfer\n"
        "1: Show checkerboard pattern\n"
        "2: Show stripe pattern\n"
        "3: Show gradient pattern\n"
        "H: Display this help message\n";
    
    strncpy(g_command_status.message, help_text, sizeof(g_command_status.message) - 1);
    return true;
}

// Status tracking
CommandStatus *command_get_status(void) {
    return &g_command_status;
}

void command_set_status(bool success, const char *message) {
    g_command_status.success = success;
    if (message) {
        strncpy(g_command_status.message, message, sizeof(g_command_status.message) - 1);
    }
}

// Debug support
void command_print_status(void) {
    printf("Command Status:\n");
    printf("  Type: %s\n", command_type_to_string(g_command_context.type));
    printf("  In Progress: %s\n", g_command_context.in_progress ? "Yes" : "No");
    printf("  Bytes Processed: %u/%u\n", g_command_context.bytes_processed, g_command_context.total_bytes);
    printf("  Success: %s\n", g_command_status.success ? "Yes" : "No");
    printf("  Duration: %ums\n", g_command_status.duration_ms);
    if (g_command_status.message[0]) {
        printf("  Message: %s\n", g_command_status.message);
    }
}

const char *command_type_to_string(CommandType type) {
    switch (type) {
        case CMD_IMAGE_START:     return "IMAGE_START";
        case CMD_IMAGE_END:       return "IMAGE_END";
        case CMD_PATTERN_CHECKER: return "PATTERN_CHECKER";
        case CMD_PATTERN_STRIPE:  return "PATTERN_STRIPE";
        case CMD_PATTERN_GRADIENT:return "PATTERN_GRADIENT";
        case CMD_HELP:           return "HELP";
        default:                 return "UNKNOWN";
    }
}
