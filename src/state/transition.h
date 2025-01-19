#ifndef DESKTHANG_STATE_TRANSITION_H
#define DESKTHANG_STATE_TRANSITION_H

#include "state.h"
#include <stdbool.h>

// State transition definition
typedef struct {
    SystemState from_state;
    SystemState to_state;
    StateCondition condition;
    bool (*validator)(void);
} StateTransition;

// Transition validation functions
bool transition_is_valid(SystemState from, SystemState to, StateCondition condition);
bool transition_execute(SystemState to_state, StateCondition condition);
const StateTransition *transition_find(SystemState from, SystemState to, StateCondition condition);

// State action execution
bool transition_execute_entry_actions(SystemState state);
bool transition_execute_exit_actions(SystemState state);
bool transition_execute_error_handler(SystemState state, void *error_context);

// Transition logging
void transition_log(SystemState from, SystemState to, StateCondition condition);
void transition_log_error(SystemState state, StateCondition condition, const char *error_msg);

// Recovery functions
typedef enum {
    RECOVERY_RESET,
    RECOVERY_RETRY,
    RECOVERY_FALLBACK,
    RECOVERY_FAIL
} RecoveryStrategy;

RecoveryStrategy transition_get_recovery_strategy(SystemState current_state);
bool transition_execute_recovery(RecoveryStrategy strategy);
uint32_t transition_calculate_backoff_delay(uint32_t retry_count);

#endif // DESKTHANG_STATE_TRANSITION_H
