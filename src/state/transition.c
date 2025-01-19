#include "transition.h"
#include "context.h"
#include <stddef.h>

// State transition matrix from protocol specification
static const StateTransition VALID_TRANSITIONS[] = {
    // Hardware Initialization
    {
        .from_state = STATE_HARDWARE_INIT,
        .to_state = STATE_DISPLAY_INIT,
        .condition = CONDITION_HARDWARE_READY,
        .validator = validate_hardware_init
    },
    {
        .from_state = STATE_HARDWARE_INIT,
        .to_state = STATE_ERROR,
        .condition = CONDITION_HARDWARE_FAILED,
        .validator = NULL
    },
    
    // Display Initialization
    {
        .from_state = STATE_DISPLAY_INIT,
        .to_state = STATE_IDLE,
        .condition = CONDITION_DISPLAY_READY,
        .validator = validate_display_init
    },
    {
        .from_state = STATE_DISPLAY_INIT,
        .to_state = STATE_ERROR,
        .condition = CONDITION_DISPLAY_FAILED,
        .validator = NULL
    },
    
    // Protocol States
    {
        .from_state = STATE_IDLE,
        .to_state = STATE_SYNCING,
        .condition = CONDITION_SYNC_RECEIVED,
        .validator = validate_sync_request
    },
    {
        .from_state = STATE_IDLE,
        .to_state = STATE_ERROR,
        .condition = CONDITION_COMMAND_FAILED,
        .validator = NULL
    },
    {
        .from_state = STATE_SYNCING,
        .to_state = STATE_READY,
        .condition = CONDITION_SYNC_VALIDATED,
        .validator = NULL
    },
    {
        .from_state = STATE_SYNCING,
        .to_state = STATE_ERROR,
        .condition = CONDITION_SYNC_FAILED,
        .validator = NULL
    },
    {
        .from_state = STATE_READY,
        .to_state = STATE_COMMAND_PROCESSING,
        .condition = CONDITION_COMMAND_RECEIVED,
        .validator = validate_command
    },
    {
        .from_state = STATE_READY,
        .to_state = STATE_DATA_TRANSFER,
        .condition = CONDITION_TRANSFER_START,
        .validator = validate_transfer
    },
    {
        .from_state = STATE_READY,
        .to_state = STATE_IDLE,
        .condition = CONDITION_COMMAND_COMPLETE,
        .validator = NULL
    },
    {
        .from_state = STATE_READY,
        .to_state = STATE_ERROR,
        .condition = CONDITION_COMMAND_FAILED,
        .validator = NULL
    },
    {
        .from_state = STATE_COMMAND_PROCESSING,
        .to_state = STATE_READY,
        .condition = CONDITION_COMMAND_COMPLETE,
        .validator = NULL
    },
    {
        .from_state = STATE_COMMAND_PROCESSING,
        .to_state = STATE_ERROR,
        .condition = CONDITION_COMMAND_FAILED,
        .validator = NULL
    },
    {
        .from_state = STATE_DATA_TRANSFER,
        .to_state = STATE_DATA_TRANSFER,
        .condition = CONDITION_TRANSFER_CHUNK,
        .validator = validate_transfer
    },
    {
        .from_state = STATE_DATA_TRANSFER,
        .to_state = STATE_READY,
        .condition = CONDITION_TRANSFER_COMPLETE,
        .validator = NULL
    },
    {
        .from_state = STATE_DATA_TRANSFER,
        .to_state = STATE_ERROR,
        .condition = CONDITION_TRANSFER_FAILED,
        .validator = NULL
    },
    {
        .from_state = STATE_ERROR,
        .to_state = STATE_IDLE,
        .condition = CONDITION_RESET,
        .validator = NULL
    },
    {
        .from_state = STATE_ERROR,
        .to_state = STATE_SYNCING,
        .condition = CONDITION_RETRY,
        .validator = NULL
    }
};

// Find matching transition in transition matrix
const StateTransition *transition_find(SystemState from, SystemState to, StateCondition condition) {
    for (size_t i = 0; i < sizeof(VALID_TRANSITIONS) / sizeof(VALID_TRANSITIONS[0]); i++) {
        const StateTransition *transition = &VALID_TRANSITIONS[i];
        if (transition->from_state == from &&
            transition->to_state == to &&
            transition->condition == condition) {
            return transition;
        }
    }
    return NULL;
}

// Validate state transition
bool transition_is_valid(SystemState from, SystemState to, StateCondition condition) {
    const StateTransition *transition = transition_find(from, to, condition);
    if (!transition) {
        return false;
    }
    
    // Execute validator if present
    if (transition->validator && !transition->validator()) {
        return false;
    }
    
    return true;
}

// Execute state transition
bool transition_execute(SystemState to_state, StateCondition condition) {
    StateContext *ctx = state_context_get();
    return state_machine_transition(to_state, condition);
}

// Execute state entry actions
bool transition_execute_entry_actions(SystemState state) {
    extern const StateActions STATE_ACTIONS[];
    if (STATE_ACTIONS[state].on_entry) {
        STATE_ACTIONS[state].on_entry();
    }
    return true;
}

// Execute state exit actions
bool transition_execute_exit_actions(SystemState state) {
    extern const StateActions STATE_ACTIONS[];
    if (STATE_ACTIONS[state].on_exit) {
        STATE_ACTIONS[state].on_exit();
    }
    return true;
}

// Execute state error handler
bool transition_execute_error_handler(SystemState state, void *error_context) {
    extern const StateActions STATE_ACTIONS[];
    if (STATE_ACTIONS[state].on_error) {
        STATE_ACTIONS[state].on_error(error_context);
    }
    return true;
}

// Log state transition
void transition_log(SystemState from, SystemState to, StateCondition condition) {
    // TODO: Implement logging
}

// Log transition error
void transition_log_error(SystemState state, StateCondition condition, const char *error_msg) {
    // TODO: Implement error logging
}

// Get recovery strategy for current state
RecoveryStrategy transition_get_recovery_strategy(SystemState current_state) {
    StateContext *ctx = state_context_get();
    
    // If we haven't exceeded retry limit, attempt retry
    if (ctx->retry_count < 8) {
        return RECOVERY_RETRY;
    }
    
    // If retries exhausted, try reset
    if (current_state != STATE_HARDWARE_INIT && 
        current_state != STATE_DISPLAY_INIT) {
        return RECOVERY_RESET;
    }
    
    // Hardware/display init failures require fallback
    return RECOVERY_FALLBACK;
}

// Execute recovery strategy
bool transition_execute_recovery(RecoveryStrategy strategy) {
    StateContext *ctx = state_context_get();
    
    switch (strategy) {
        case RECOVERY_RESET:
            state_context_reset();
            return transition_execute(STATE_IDLE, CONDITION_RESET);
            
        case RECOVERY_RETRY:
            if (!state_context_increment_retry()) {
                return false;
            }
            return transition_execute(STATE_SYNCING, CONDITION_RETRY);
            
        case RECOVERY_FALLBACK:
            // TODO: Implement fallback behavior
            return false;
            
        case RECOVERY_FAIL:
            return false;
    }
    
    return false;
}

// Calculate exponential backoff delay
uint32_t transition_calculate_backoff_delay(uint32_t retry_count) {
    // Base delay of 50ms with exponential backoff up to 1000ms
    uint32_t delay = 50;
    for (uint32_t i = 0; i < retry_count && delay < 1000; i++) {
        delay *= 2;
    }
    return delay;
}
