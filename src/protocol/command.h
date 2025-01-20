#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../common/deskthang_constants.h"
#include "protocol.h"
#include "packet.h"

// Command types
typedef enum {
    CMD_IMAGE_START = 'I',    // Start image transfer (RGB565 format, DISPLAY_WIDTH×DISPLAY_HEIGHT)
    CMD_IMAGE_DATA = 'D',     // Image data chunk
    CMD_IMAGE_END = 'E',      // End image transfer
    CMD_PATTERN_CHECKER = '1', // Show checkerboard pattern
    CMD_PATTERN_STRIPE = '2',  // Show stripe pattern
    CMD_PATTERN_GRADIENT = '3',// Show gradient pattern
    CMD_HELP = 'H',           // Display help/command list
    CMD_PING = 'P'            // Ping command for testing
} CommandType;

// Command context for tracking multi-packet commands
typedef struct {
    CommandType type;          // Active command
    uint32_t start_time;       // Command start timestamp
    uint32_t bytes_processed;  // Progress tracking
    uint32_t total_bytes;      // Should be TRANSFER_MAX_SIZE for image transfers
    size_t data_size;         // Size of current data chunk
    bool in_progress;          // Command is active
    void *command_data;        // Command-specific data
} CommandContext;

// Command initialization and management
bool command_init(void);
void command_reset(void);
CommandContext *command_get_context(void);

// Command processing
bool command_process(const uint8_t *data, size_t len);
bool command_complete(void);
bool command_abort(void);

// Command validation
bool command_validate_type(CommandType type);
bool command_validate_state(void);
bool command_validate_sequence(const Packet *packet);

// Image transfer commands
bool command_start_image_transfer(const uint8_t *data, size_t len);
bool command_end_image_transfer(void);

// Pattern commands
bool command_show_checkerboard(void);
bool command_show_stripes(void);
bool command_show_gradient(void);

// Help command
bool command_show_help(void);

// Command status
typedef struct {
    bool success;
    uint32_t duration_ms;
    uint32_t bytes_processed;
    char message[DEBUG_MESSAGE_MAX];  // Use debug message size constant
} CommandStatus;

// Status tracking
CommandStatus *command_get_status(void);
void command_set_status(bool success, const char *message);

// Debug support
void command_print_status(void);
const char *command_type_to_string(CommandType type);

bool command_type_valid(void);
bool command_params_valid(void);
bool command_resources_available(void);

// Add ping command prototype
bool command_ping(void);

#endif // COMMAND_H
