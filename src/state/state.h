#ifndef DESKTHANG_STATE_H
#define DESKTHANG_STATE_H

#include <stdbool.h>
#include <stdint.h>

// System states as defined in protocol
typedef enum {
    STATE_HARDWARE_INIT,
    STATE_DISPLAY_INIT,
    STATE_IDLE,
    STATE_SYNCING,
    STATE_READY,
    STATE_COMMAND_PROCESSING,
    STATE_DATA_TRANSFER,
    STATE_ERROR
} SystemState;

// State transition conditions
typedef enum {
    CONDITION_HARDWARE_READY,
    CONDITION_HARDWARE_FAILED,
    CONDITION_DISPLAY_READY,
    CONDITION_DISPLAY_FAILED,
    CONDITION_SYNC_RECEIVED,
    CONDITION_SYNC_VALIDATED,
    CONDITION_SYNC_FAILED,
    CONDITION_COMMAND_RECEIVED,
    CONDITION_COMMAND_COMPLETE,
    CONDITION_COMMAND_FAILED,
    CONDITION_TRANSFER_START,
    CONDITION_TRANSFER_CHUNK,
    CONDITION_TRANSFER_COMPLETE,
    CONDITION_TRANSFER_FAILED,
    CONDITION_RESET,
    CONDITION_RETRY
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
bool state_machine_transition(SystemState next_state, StateCondition condition);
SystemState state_machine_get_current(void);
SystemState state_machine_get_previous(void);
uint32_t state_machine_get_time_in_state(void);
bool state_machine_is_valid_transition(SystemState from, SystemState to, StateCondition condition);

// State validation functions
bool validate_hardware_init(void);
bool validate_display_init(void);
bool validate_sync_request(void);
bool validate_command(void);
bool validate_transfer(void);

#endif // DESKTHANG_STATE_H
