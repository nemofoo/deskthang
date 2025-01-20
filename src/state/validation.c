#include "validation.h"
#include "../error/logging.h"
#include "../system/time.h"
#include <string.h>

// Constants
#define MAX_HISTORY_ENTRIES 16
#define HISTORY_MASK (MAX_HISTORY_ENTRIES - 1)

// State requirements table
static const StateRequirements STATE_REQUIREMENTS[] = {
    [STATE_HARDWARE_INIT] = {
        .min_memory = 1024,          // 1KB minimum
        .max_duration = 5000,        // 5 seconds max
        .min_duration = 0,           // No minimum
        .max_retries = 3,            // 3 retries max
        .required_flags = STATE_VALID_RESOURCES
    },
    [STATE_DISPLAY_INIT] = {
        .min_memory = 2048,          // 2KB minimum
        .max_duration = 10000,       // 10 seconds max
        .min_duration = 0,           // No minimum
        .max_retries = 3,            // 3 retries max
        .required_flags = STATE_VALID_RESOURCES | STATE_VALID_HISTORY
    },
    [STATE_IDLE] = {
        .min_memory = 512,           // 512B minimum
        .max_duration = 0,           // No maximum
        .min_duration = 0,           // No minimum
        .max_retries = 0,            // No retries
        .required_flags = STATE_VALID_RESOURCES
    },
    [STATE_SYNCING] = {
        .min_memory = 1024,          // 1KB minimum
        .max_duration = 30000,       // 30 seconds max
        .min_duration = 0,           // No minimum
        .max_retries = 5,            // 5 retries max
        .required_flags = STATE_VALID_RESOURCES | STATE_VALID_TIMING
    },
    [STATE_READY] = {
        .min_memory = 1024,          // 1KB minimum
        .max_duration = 0,           // No maximum
        .min_duration = 0,           // No minimum
        .max_retries = 0,            // No retries
        .required_flags = STATE_VALID_RESOURCES | STATE_VALID_HISTORY
    },
    [STATE_COMMAND_PROCESSING] = {
        .min_memory = 2048,          // 2KB minimum
        .max_duration = 5000,        // 5 seconds max
        .min_duration = 0,           // No minimum
        .max_retries = 2,            // 2 retries max
        .required_flags = STATE_VALID_ALL
    },
    [STATE_DATA_TRANSFER] = {
        .min_memory = 4096,          // 4KB minimum
        .max_duration = 60000,       // 60 seconds max
        .min_duration = 0,           // No minimum
        .max_retries = 3,            // 3 retries max
        .required_flags = STATE_VALID_ALL
    },
    [STATE_ERROR] = {
        .min_memory = 512,           // 512B minimum
        .max_duration = 0,           // No maximum
        .min_duration = 0,           // No minimum
        .max_retries = 0,            // No retries
        .required_flags = STATE_VALID_NONE
    }
};

// State history circular buffer
static StateHistoryEntry state_history[MAX_HISTORY_ENTRIES];
static uint32_t history_write_index = 0;
static uint32_t history_count = 0;

// State validation functions
bool validate_state_entry(const StateContext *ctx, SystemState state) {
    if (!ctx) return false;

    // Check if we have the required flags for this state
    uint32_t current_flags = STATE_VALID_NONE;
    
    if (state_check_resources(state)) {
        current_flags |= STATE_VALID_RESOURCES;
    }
    
    if (state_check_timing_constraints(ctx, state)) {
        current_flags |= STATE_VALID_TIMING;
    }
    
    if (state_history_validate_sequence()) {
        current_flags |= STATE_VALID_HISTORY;
    }

    // Entry conditions are met if we have all required flags
    if ((current_flags & STATE_REQUIREMENTS[state].required_flags) == 
        STATE_REQUIREMENTS[state].required_flags) {
        return true;
    }

    // Log which requirements weren't met
    char message[256];
    snprintf(message, sizeof(message),
            "Entry validation failed for %s - Required: 0x%x, Current: 0x%x",
            state_to_string(state),
            STATE_REQUIREMENTS[state].required_flags,
            current_flags);
    logging_write("StateValidation", message);
    
    return false;
}

bool validate_state_resources(const StateContext *ctx, SystemState state) {
    if (!ctx) return false;
    return state_check_resources(state);
}

bool validate_state_timing(const StateContext *ctx, SystemState state) {
    if (!ctx) return false;
    return state_check_timing_constraints(ctx, state);
}

bool validate_state_history(const StateContext *ctx, SystemState state) {
    if (!ctx) return false;
    
    // Always allow error state
    if (state == STATE_ERROR) return true;
    
    // Check if we have enough history
    if (history_count == 0) {
        return state == STATE_HARDWARE_INIT;  // Only hardware init allowed with no history
    }
    
    const StateHistoryEntry *last = state_history_get_last();
    if (!last) return false;
    
    // Validate state sequence
    return state_history_validate_sequence();
}

bool validate_state_all(const StateContext *ctx, SystemState state) {
    return validate_state_entry(ctx, state) &&
           validate_state_resources(ctx, state) &&
           validate_state_timing(ctx, state) &&
           validate_state_history(ctx, state);
}

// History management
void state_history_add(SystemState state, StateCondition entry_condition) {
    StateHistoryEntry *entry = &state_history[history_write_index];
    entry->state = state;
    entry->entry_time = get_system_time();
    entry->exit_time = 0;  // Will be set on exit
    entry->entry_condition = entry_condition;
    entry->exit_condition = CONDITION_NONE;
    
    history_write_index = (history_write_index + 1) & HISTORY_MASK;
    if (history_count < MAX_HISTORY_ENTRIES) {
        history_count++;
    }
}

void state_history_update_exit(StateCondition exit_condition) {
    if (history_count > 0) {
        uint32_t last_index = (history_write_index - 1) & HISTORY_MASK;
        state_history[last_index].exit_time = get_system_time();
        state_history[last_index].exit_condition = exit_condition;
    }
}

const StateHistoryEntry *state_history_get_last(void) {
    if (history_count == 0) return NULL;
    uint32_t last_index = (history_write_index - 1) & HISTORY_MASK;
    return &state_history[last_index];
}

bool state_history_validate_sequence(void) {
    if (history_count == 0) return true;  // No history is valid
    
    const StateHistoryEntry *last = state_history_get_last();
    if (!last) return false;
    
    // Check for invalid transitions in history
    for (uint32_t i = 1; i < history_count && i < MAX_HISTORY_ENTRIES; i++) {
        uint32_t idx = (history_write_index - i - 1) & HISTORY_MASK;
        uint32_t next_idx = (history_write_index - i) & HISTORY_MASK;
        
        // Check if this transition was valid
        if (!state_machine_validate_transition(
                state_history[idx].state,
                state_history[next_idx].state,
                state_history[next_idx].entry_condition)) {
            return false;
        }
    }
    
    return true;
}

// Resource management
bool state_check_resources(SystemState state) {
    // TODO: Implement actual memory checking
    // For now, assume we have enough memory
    return true;
}

void state_update_resource_usage(void) {
    // TODO: Implement resource usage tracking
}

// Timing management
bool state_check_timing_constraints(const StateContext *ctx, SystemState state) {
    if (!ctx) return false;
    
    uint32_t current_time = get_system_time();
    uint32_t duration = current_time - ctx->entry_time;
    
    // Check maximum duration if set
    if (STATE_REQUIREMENTS[state].max_duration > 0 &&
        duration > STATE_REQUIREMENTS[state].max_duration) {
        char message[256];
        snprintf(message, sizeof(message),
                "State %s exceeded max duration: %lu > %lu",
                state_to_string(state),
                duration,
                STATE_REQUIREMENTS[state].max_duration);
        logging_write("StateValidation", message);
        return false;
    }
    
    // Check minimum duration if set
    if (STATE_REQUIREMENTS[state].min_duration > 0 &&
        duration < STATE_REQUIREMENTS[state].min_duration) {
        char message[256];
        snprintf(message, sizeof(message),
                "State %s hasn't met min duration: %lu < %lu",
                state_to_string(state),
                duration,
                STATE_REQUIREMENTS[state].min_duration);
        logging_write("StateValidation", message);
        return false;
    }
    
    // Check retry count if applicable
    if (STATE_REQUIREMENTS[state].max_retries > 0 &&
        ctx->retry_count > STATE_REQUIREMENTS[state].max_retries) {
        char message[256];
        snprintf(message, sizeof(message),
                "State %s exceeded max retries: %lu > %lu",
                state_to_string(state),
                ctx->retry_count,
                STATE_REQUIREMENTS[state].max_retries);
        logging_write("StateValidation", message);
        return false;
    }
    
    return true;
}

uint32_t state_get_remaining_time(const StateContext *ctx, SystemState state) {
    if (!ctx || STATE_REQUIREMENTS[state].max_duration == 0) {
        return 0;
    }
    
    uint32_t current_time = get_system_time();
    uint32_t duration = current_time - ctx->entry_time;
    
    if (duration >= STATE_REQUIREMENTS[state].max_duration) {
        return 0;
    }
    
    return STATE_REQUIREMENTS[state].max_duration - duration;
}

bool state_validate_transition(SystemState current, SystemState next, StateCondition condition) {
    // Check if states are valid
    if (!state_validate_state(current) || !state_validate_state(next)) {
        return false;
    }

    // Check if transition is allowed
    if (!state_validate_entry_conditions(next)) {
        return false;
    }

    // Check timing constraints
    if (!state_validate_timing(next)) {
        return false;
    }

    // Check state history
    if (!state_validate_history(next)) {
        return false;
    }

    return true;
}

bool state_validate_timing(SystemState state) {
    const StateContext *ctx = state_context_get();
    return state_check_timing_constraints(ctx, state);
}

bool state_validate_history(SystemState state) {
    // Check if we're transitioning to a valid next state based on history
    const StateContext *ctx = state_context_get();
    return state_history_is_valid_transition(ctx->current_state, state);
} 