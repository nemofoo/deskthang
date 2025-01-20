#ifndef __DESKTHANG_STATE_H
#define __DESKTHANG_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "../common/deskthang_constants.h"

// System states as defined in protocol_state_machine.md
typedef enum {
    STATE_HARDWARE_INIT,     // Initial hardware setup
    STATE_DISPLAY_INIT,      // Display initialization
    STATE_IDLE,             // Default waiting state
    STATE_SYNCING,          // Protocol synchronization
    STATE_READY,            // Ready for commands
    STATE_COMMAND_PROCESSING, // Processing command
    STATE_DATA_TRANSFER,    // Handling data transfer
    STATE_ERROR             // Error handling state
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
    CONDITION_ERROR,
    CONDITION_RESET,
    CONDITION_RETRY,
    CONDITION_RECOVERED
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

// Core state machine functions
bool state_machine_init(void);
SystemState state_machine_get_current(void);
bool state_machine_transition(SystemState next_state, StateCondition condition);
bool state_machine_can_transition_to(SystemState next_state);

// State validation
bool state_machine_validate_state(SystemState state);
bool state_machine_validate_transition(SystemState current, SystemState next, StateCondition condition);

// State history
SystemState state_machine_get_previous(void);
uint32_t state_machine_get_state_duration(void);

// Debug support
const char *state_to_string(SystemState state);
const char *condition_to_string(StateCondition condition);

bool state_machine_handle_error(void);
SystemState state_machine_get_current_state(void);

// Use STATE_* constants for validation flags
typedef struct {
    uint8_t flags;          // Use STATE_VALID_* flags
    uint32_t entry_time;    
    uint32_t duration;      
    uint8_t retry_count;    
} StateValidation;

// Make STATE_ACTIONS available to other files
extern const StateActions STATE_ACTIONS[];

#endif // __DESKTHANG_STATE_H
