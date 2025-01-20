#include "../system/time.h"
#include "command.h"
#include "../state/state.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../hardware/display.h"

// Global command context
static CommandContext g_command_context = {0};
static CommandStatus g_command_status = {0};

// Initialize command processing
bool command_init(void) {
    memset(&g_command_context, 0, sizeof(CommandContext));
    g_command_context.in_progress = false;
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
bool command_process(const uint8_t *data, size_t len) {
    if (!data || !len) {
        return false;
    }

    // Validate command state
    if (!command_validate_state()) {
        return false;
    }

    // Track timing
    g_command_context.start_time = deskthang_time_get_ms();
    g_command_context.type = (CommandType)data[0];
    g_command_context.in_progress = true;

    // Process command
    bool result = false;
    switch (data[0]) {
        case CMD_IMAGE_START:
            result = command_start_image_transfer(data + 1, len - 1);
            break;
            
        case CMD_IMAGE_END:
            result = command_end_image_transfer();
            break;
            
        case CMD_PATTERN_CHECKER:
            result = command_show_checkerboard();
            break;
            
        case CMD_PATTERN_STRIPE:
            result = command_show_stripes();
            break;
            
        case CMD_PATTERN_GRADIENT:
            result = command_show_gradient();
            break;
            
        case CMD_HELP:
            result = command_show_help();
            break;
            
        default:
            command_set_status(false, "Unknown command");
            return false;
    }

    return result;
}

// Complete current command
bool command_complete(void) {
    if (!g_command_context.in_progress) {
        return false;
    }
    
    // Update status
    g_command_status.duration_ms = deskthang_time_get_ms() - g_command_context.start_time;
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
bool command_start_image_transfer(const uint8_t *data, size_t len) {
    // Validate transfer parameters here
    
    // Transition to transfer state
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
    // Validate transfer completion here
    
    // Return to ready state
    return state_machine_transition(STATE_READY, CONDITION_TRANSFER_COMPLETE);
}

// Pattern commands
bool command_show_checkerboard(void) {
    bool result = display_draw_test_pattern(TEST_PATTERN_CHECKERBOARD, 0);
    command_set_status(result, result ? "Checkerboard pattern displayed" : "Failed to display checkerboard pattern");
    return result;
}

bool command_show_stripes(void) {
    bool result = display_draw_test_pattern(TEST_PATTERN_COLOR_BARS, 0);
    command_set_status(result, result ? "Color bars pattern displayed" : "Failed to display color bars pattern");
    return result;
}

bool command_show_gradient(void) {
    bool result = display_draw_test_pattern(TEST_PATTERN_GRADIENT, 0);
    command_set_status(result, result ? "Gradient pattern displayed" : "Failed to display gradient pattern");
    return result;
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
    printf("  In Progress: %s\n", g_command_context.in_progress ? "Yes" : "No");
    printf("  Command Type: %s\n", command_type_to_string(g_command_context.type));
    printf("  Bytes Processed: %lu\n", (unsigned long)g_command_context.bytes_processed);
    printf("  Total Bytes: %lu\n", (unsigned long)g_command_context.total_bytes);
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

// Add these implementations
bool command_type_valid(void) {
    // TODO: Implement proper validation
    return true;
}

bool command_params_valid(void) {
    // TODO: Implement proper validation
    return true;
}

bool command_resources_available(void) {
    // TODO: Implement proper validation
    return true;
}
