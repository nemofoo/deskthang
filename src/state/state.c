#include <stdio.h>
#include "state.h"
#include "context.h"
#include "transition.h"
#include "../error/logging.h"
#include "../error/recovery.h"
#include "../system/time.h"
#include "../protocol/protocol.h"
#include "../protocol/command.h"
#include "../protocol/transfer.h"
#include "../debug/debug.h"
#include "../hardware/hardware.h"
#include "../hardware/display.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"

// External configurations
extern const HardwareConfig hw_config;
extern const DisplayConfig display_config;

// Forward declare hardware functions we need
bool spi_is_configured(void);
bool gpio_pins_configured(void);
bool timing_requirements_met(void);
bool display_reset_complete(void);
bool display_params_valid(void);
bool display_responding(void);

// Forward declare protocol functions
bool protocol_validate_version(uint8_t version);
bool protocol_timing_valid(void);
bool command_type_valid(void);
bool command_params_valid(void);
bool command_resources_available(void);
bool transfer_buffer_available(void);
bool transfer_sequence_valid(void);
bool transfer_checksum_valid(void);

// Add after includes
extern void log_info(const char *format, ...);  // From logging.h

// Forward declarations for state handlers
static void hardware_init_entry(void);
static void hardware_init_exit(void);
static void hardware_init_error(void *ctx);
static void display_init_entry(void);
static void display_init_exit(void);
static void display_init_error(void *ctx);
static void idle_entry(void);
static void idle_exit(void);
static void idle_error(void *ctx);
static void syncing_entry(void);
static void syncing_exit(void);
static void syncing_error(void *ctx);
static void ready_entry(void);
static void ready_exit(void);
static void ready_error(void *ctx);
static void command_entry(void);
static void command_exit(void);
static void command_error(void *ctx);
static void transfer_entry(void);
static void transfer_exit(void);
static void transfer_error(void *ctx);
static void error_entry(void);
static void error_exit(void);
static void error_error(void *ctx);

// State action handlers for each state
const StateActions STATE_ACTIONS[] = {
    [STATE_HARDWARE_INIT] = {
        .on_entry = hardware_init_entry,
        .on_exit = hardware_init_exit,
        .on_error = hardware_init_error
    },
    [STATE_DISPLAY_INIT] = {
        .on_entry = display_init_entry,
        .on_exit = display_init_exit,
        .on_error = display_init_error
    },
    [STATE_IDLE] = {
        .on_entry = idle_entry,
        .on_exit = idle_exit,
        .on_error = idle_error
    },
    [STATE_SYNCING] = {
        .on_entry = syncing_entry,
        .on_exit = syncing_exit,
        .on_error = syncing_error
    },
    [STATE_READY] = {
        .on_entry = ready_entry,
        .on_exit = ready_exit,
        .on_error = ready_error
    },
    [STATE_COMMAND_PROCESSING] = {
        .on_entry = command_entry,
        .on_exit = command_exit,
        .on_error = command_error
    },
    [STATE_DATA_TRANSFER] = {
        .on_entry = transfer_entry,
        .on_exit = transfer_exit,
        .on_error = transfer_error
    },
    [STATE_ERROR] = {
        .on_entry = error_entry,
        .on_exit = error_exit,
        .on_error = error_error
    }
};

// State action handler implementations
static void hardware_init_entry(void) {
    printf("State: Entering HARDWARE_INIT\n");
    
    // Initialize hardware
    if (!hardware_init(&hw_config)) {
        printf("State: Hardware initialization failed\n");
        state_machine_transition(STATE_ERROR, CONDITION_ERROR);
        return;
    }
    
    // Hardware init successful, move to display init
    printf("State: Hardware initialization successful\n");
    state_machine_transition(STATE_DISPLAY_INIT, CONDITION_HARDWARE_READY);
}

static void hardware_init_exit(void) {
    // Nothing to clean up
}

static void hardware_init_error(void *ctx) {
    // Handle hardware initialization errors
    printf("State: Hardware initialization error\n");
    // Try to recover by reinitializing
    if (!hardware_reset()) {
        printf("State: Hardware reset failed\n");
        state_machine_transition(STATE_ERROR, CONDITION_ERROR);
    }
}

static void display_init_entry(void) {
    printf("State: Entering DISPLAY_INIT\n");
    
    // Initialize display
    if (!display_init(&hw_config, &display_config)) {
        printf("State: Display initialization failed\n");
        state_machine_transition(STATE_ERROR, CONDITION_ERROR);
        return;
    }
    
    // Display init successful, move to idle
    printf("State: Display initialization successful\n");
    state_machine_transition(STATE_IDLE, CONDITION_DISPLAY_READY);
}

static void display_init_exit(void) {
    // Clean up if needed
}

static void display_init_error(void *ctx) {
    // Handle display initialization errors
}

static void idle_entry(void) {
    // Clear transfer buffers
    // Reset sequence counter
    // Initialize packet parser
}

static void idle_exit(void) {
    // Clean up if needed
}

static void idle_error(void *ctx) {
    // Handle idle state errors
}

static void syncing_entry(void) {
    // Send/verify SYNC
    // Start sync timer
    // Initialize retry counter
}

static void syncing_exit(void) {
    // Clean up if needed
}

static void syncing_error(void *ctx) {
    // Handle syncing errors
}

static void ready_entry(void) {
    // Reset command parser
    // Initialize transfer state
    // Start command timer
}

static void ready_exit(void) {
    // Clean up if needed
}

static void ready_error(void *ctx) {
    // Handle ready state errors
}

static void command_entry(void) {
    // Parse command parameters
    // Validate command type
    // Initialize command context
}

static void command_exit(void) {
    // Clean up if needed
}

static void command_error(void *ctx) {
    // Handle command processing errors
}

static void transfer_entry(void) {
    // Initialize transfer buffer
    // Reset chunk counter
    // Start transfer timer
}

static void transfer_exit(void) {
    // Clean up if needed
}

static void transfer_error(void *ctx) {
    // Handle transfer errors
}

static void error_entry(void) {
    // Log error context
    // Initialize recovery
    // Stop active transfers
}

static void error_exit(void) {
    // Clean up if needed
}

static void error_error(void *ctx) {
    // Handle errors in error state
}

// Global state context
static StateContext g_state_context;

bool state_machine_init(void) {
    // Initialize context
    state_context_init();
    
    // Set initial state
    g_state_context.current_state = STATE_HARDWARE_INIT;
    g_state_context.previous_state = STATE_HARDWARE_INIT;
    g_state_context.last_condition = CONDITION_NONE;
    g_state_context.last_update = deskthang_time_get_ms();
    
    // Execute entry action for initial state
    if (STATE_ACTIONS[STATE_HARDWARE_INIT].on_entry) {
        STATE_ACTIONS[STATE_HARDWARE_INIT].on_entry();
    }
    
    return true;
}

bool state_machine_transition(SystemState next_state, StateCondition condition) {
    // Get current state
    SystemState current = state_machine_get_current();

    printf("State: Attempting transition from %s to %s (condition: %s)\n",
           state_to_string(current),
           state_to_string(next_state),
           condition_to_string(condition));

    // Validate transition
    if (!state_machine_validate_transition(current, next_state, condition)) {
        printf("State Error: Invalid transition from %s to %s\n",
               state_to_string(current),
               state_to_string(next_state));
        logging_write("State", "Invalid state transition");
        return false;
    }

    // Execute exit actions for current state
    if (STATE_ACTIONS[current].on_exit) {
        printf("State: Executing exit actions for %s\n", state_to_string(current));
        STATE_ACTIONS[current].on_exit();
    }

    // Update state
    g_state_context.previous_state = current;
    g_state_context.current_state = next_state;
    g_state_context.last_condition = condition;
    g_state_context.last_update = deskthang_time_get_ms();

    // Execute entry actions for new state
    if (STATE_ACTIONS[next_state].on_entry) {
        printf("State: Executing entry actions for %s\n", state_to_string(next_state));
        STATE_ACTIONS[next_state].on_entry();
    }

    printf("State: Successfully transitioned to %s\n", state_to_string(next_state));
    logging_write("State", "State transition complete");

    return true;
}

bool state_machine_handle_error(void) {
    if (g_state_context.current_state == STATE_ERROR) {
        return false;  // Already in error state
    }
    
    debug_log_transition(g_state_context.current_state, STATE_ERROR, CONDITION_ERROR, true);
    return state_machine_transition(STATE_ERROR, CONDITION_ERROR);
}

bool state_machine_attempt_recovery(void) {
    if (g_state_context.current_state != STATE_ERROR) {
        return false;
    }
    
    if (state_context_can_retry()) {
        state_context_increment_retry();
        debug_log_retry("state_recovery");
        return state_machine_transition(g_state_context.previous_state, CONDITION_RECOVERED);
    }
    
    return state_machine_transition(STATE_IDLE, CONDITION_RESET);
}

SystemState state_machine_get_current(void) {
    return g_state_context.current_state;
}

SystemState state_machine_get_previous(void) {
    return g_state_context.previous_state;
}

bool state_machine_is_in_error(void) {
    return g_state_context.current_state == STATE_ERROR;
}

// Debug support
const char *state_to_string(SystemState state) {
    switch (state) {
        case STATE_HARDWARE_INIT:     return "HARDWARE_INIT";
        case STATE_DISPLAY_INIT:      return "DISPLAY_INIT";
        case STATE_IDLE:             return "IDLE";
        case STATE_SYNCING:          return "SYNCING";
        case STATE_READY:            return "READY";
        case STATE_COMMAND_PROCESSING:return "COMMAND_PROCESSING";
        case STATE_DATA_TRANSFER:    return "DATA_TRANSFER";
        case STATE_ERROR:            return "ERROR";
        default:                     return "UNKNOWN";
    }
}

const char *condition_to_string(StateCondition condition) {
    switch (condition) {
        case CONDITION_NONE:           return "NONE";
        case CONDITION_HARDWARE_READY: return "HARDWARE_READY";
        case CONDITION_DISPLAY_READY:  return "DISPLAY_READY";
        case CONDITION_SYNC_RECEIVED:  return "SYNC_RECEIVED";
        case CONDITION_SYNC_VALID:     return "SYNC_VALID";
        case CONDITION_COMMAND_VALID:  return "COMMAND_VALID";
        case CONDITION_TRANSFER_START: return "TRANSFER_START";
        case CONDITION_TRANSFER_COMPLETE: return "TRANSFER_COMPLETE";
        case CONDITION_ERROR:          return "ERROR";
        case CONDITION_RECOVERED:      return "RECOVERED";
        case CONDITION_RESET:          return "RESET";
        case CONDITION_RETRY:          return "RETRY";
        default:                       return "UNKNOWN";
    }
}

// State validation functions implementation
bool validate_hardware_init(void) {
    // Validate SPI configuration
    if (!spi_is_configured()) {
        logging_write("State", "SPI not configured");
        return false;
    }

    // Validate GPIO pins
    if (!gpio_pins_configured()) {
        logging_write("State", "GPIO pins not configured");
        return false;
    }

    // Validate timing requirements
    if (!timing_requirements_met()) {
        logging_write("State", "Timing requirements not met");
        return false;
    }

    return true;
}

bool validate_display_init(void) {
    // Validate reset sequence complete
    if (!display_reset_complete()) {
        logging_write("State", "Display reset incomplete");
        return false;
    }

    // Validate display parameters
    if (!display_params_valid()) {
        logging_write("State", "Display parameters invalid");
        return false;
    }

    // Validate display response
    if (!display_responding()) {
        logging_write("State", "Display not responding");
        return false;
    }

    return true;
}

bool validate_sync_request(void) {
    // Get protocol config
    const ProtocolConfig *config = protocol_get_config();
    if (!config) {
        logging_write("State", "Protocol config not available");
        return false;
    }

    // Validate protocol version
    if (!protocol_validate_version(config->version)) {
        logging_write("State", "Invalid protocol version");
        return false;
    }

    // Validate timing within limits
    if (!protocol_timing_valid()) {
        logging_write("State", "Protocol timing invalid");
        return false;
    }

    // Get retry count from context
    uint32_t retry_count = state_context_get_retry_count();
    if (retry_count >= config->timing.max_retries) {
        logging_write("State", "Max retries exceeded");
        return false;
    }

    return true;
}

bool validate_command(void) {
    // Validate command type
    if (!command_type_valid()) {
        logging_write("State", "Invalid command type");
        return false;
    }

    // Validate parameters
    if (!command_params_valid()) {
        logging_write("State", "Invalid command parameters");
        return false;
    }

    // Validate resources available
    if (!command_resources_available()) {
        logging_write("State", "Required resources unavailable");
        return false;
    }

    return true;
}

bool validate_transfer(void) {
    // Validate buffer space available
    if (!transfer_buffer_available()) {
        logging_write("State", "Transfer buffer full");
        return false;
    }

    // Validate sequence numbers
    if (!transfer_sequence_valid()) {
        logging_write("State", "Invalid transfer sequence");
        return false;
    }

    // Validate checksums match
    if (!transfer_checksum_valid()) {
        logging_write("State", "Invalid transfer checksum");
        return false;
    }

    return true;
}

bool state_machine_handle_recovery(const ErrorDetails *error) {
    if (!error) {
        return false;
    }
    
    RecoveryStrategy strategy = recovery_get_strategy(error);
    char context[256];
    snprintf(context, sizeof(context),
             "Error: %s, Strategy: %s",
             error->message,
             recovery_strategy_to_string(strategy));
    logging_write_with_context("StateMachine", "Recovery attempt", context);
    
    switch (strategy) {
        case RECOVERY_RETRY:
            return state_machine_transition(STATE_SYNCING, CONDITION_RETRY);
            
        case RECOVERY_RESET_STATE:
            return state_machine_transition(STATE_IDLE, CONDITION_RESET);
            
        case RECOVERY_REINIT:
            if (error->type == ERROR_TYPE_HARDWARE) {
                return state_machine_transition(STATE_DISPLAY_INIT, CONDITION_RESET);
            } else {
                return state_machine_transition(STATE_HARDWARE_INIT, CONDITION_RESET);
            }
            
        case RECOVERY_REBOOT:
            if (recovery_get_config()->allow_reboot) {
                reset_usb_boot(0, 0);  // Use Pico SDK's reset function
                return true; // Never reached
            }
            return false;
            
        default:
            return false;
    }
}

bool state_machine_validate_state(SystemState state) {
    return state >= STATE_HARDWARE_INIT && state <= STATE_ERROR;
}

bool state_machine_validate_transition(SystemState current, SystemState next, StateCondition condition) {
    // First validate the states themselves
    if (!state_machine_validate_state(current) || !state_machine_validate_state(next)) {
        printf("State Error: Invalid state value(s) in transition\n");
        return false;
    }

    // Define valid transitions
    switch (current) {
        case STATE_HARDWARE_INIT:
            // Can only go to DISPLAY_INIT or ERROR
            return (next == STATE_DISPLAY_INIT && condition == CONDITION_HARDWARE_READY) ||
                   (next == STATE_ERROR && condition == CONDITION_ERROR);

        case STATE_DISPLAY_INIT:
            // Can go to IDLE or ERROR
            return (next == STATE_IDLE && condition == CONDITION_DISPLAY_READY) ||
                   (next == STATE_ERROR && condition == CONDITION_ERROR);

        case STATE_IDLE:
            // Can go to SYNCING or ERROR
            return (next == STATE_SYNCING && condition == CONDITION_SYNC_RECEIVED) ||
                   (next == STATE_ERROR && condition == CONDITION_ERROR);

        case STATE_ERROR:
            // Can attempt recovery to previous state or reset to IDLE
            return (next == g_state_context.previous_state && condition == CONDITION_RECOVERED) ||
                   (next == STATE_IDLE && condition == CONDITION_RESET);

        default:
            printf("State Error: Unhandled current state in transition validation\n");
            return false;
    }
}
