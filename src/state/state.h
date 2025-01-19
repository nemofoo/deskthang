#ifndef DESKTHANG_STATE_H
#define DESKTHANG_STATE_H

#include <stdint.h>
#include <stdbool.h>

// System states
typedef enum {
    STATE_HARDWARE_INIT,    // Initial hardware setup
    STATE_DISPLAY_INIT,     // Display initialization
    STATE_IDLE,            // Waiting for connection
    STATE_SYNCING,         // Establishing sync
    STATE_READY,           // Ready for commands
    STATE_COMMAND_PROCESSING, // Processing command
    STATE_DATA_TRANSFER,   // Transferring data
    STATE_ERROR            // Error handling
} SystemState;

// State transition conditions
typedef enum {
    CONDITION_NONE,
    CONDITION_HARDWARE_READY,
    CONDITION_DISPLAY_READY,
    CONDITION_SYNC_RECEIVED,
    CONDITION_SYNC_VALID,
    CONDITION_COMMAND_VALID,
    CONDITION_TRANSFER_START,
    CONDITION_TRANSFER_COMPLETE,
    CONDITION_ERROR,        // Error occurred
    CONDITION_RECOVERED,    // Recovery successful
    CONDITION_RESET,        // System reset needed
    CONDITION_RETRY         // Retry operation
} StateCondition;

// Function pointer types for state actions
typedef void (*StateEntryAction)(void);
typedef void (*StateExitAction)(void);
typedef void (*StateErrorHandler)(void *error_context);

// State action handlers
typedef struct {
    StateEntryAction on_entry;
    StateExitAction on_exit;
    StateErrorHandler on_error;
} StateActions;

// State machine initialization
bool state_machine_init(void);

// State transitions
bool state_machine_transition(SystemState next_state, StateCondition condition);
bool state_machine_handle_error(void);
bool state_machine_attempt_recovery(void);

// State queries
SystemState state_machine_get_current(void);
SystemState state_machine_get_previous(void);
bool state_machine_is_in_error(void);

// Debug support
const char *state_to_string(SystemState state);
const char *condition_to_string(StateCondition condition);

#endif // DESKTHANG_STATE_H
