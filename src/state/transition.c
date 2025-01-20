#include "transition.h"
#include "../error/logging.h"
#include "../system/time.h"
#include <stdio.h>

// Add at top of file after includes
extern const StateActions STATE_ACTIONS[];

// Forward declarations of validation functions
static bool validate_hardware_init(void);
static bool validate_display_init(void);
static bool validate_sync_request(void);
static bool validate_command(void);
static bool validate_transfer(void);
static bool validate_error_entry(void);
static bool validate_error_recovery(void);

// Valid state transitions table with validators
static const StateTransition VALID_TRANSITIONS[] = {
    // Hardware init transitions
    {STATE_HARDWARE_INIT, STATE_DISPLAY_INIT, CONDITION_HARDWARE_READY, validate_hardware_init},
    {STATE_HARDWARE_INIT, STATE_ERROR, CONDITION_ERROR, validate_error_entry},
    
    // Display init transitions
    {STATE_DISPLAY_INIT, STATE_IDLE, CONDITION_DISPLAY_READY, validate_display_init},
    {STATE_DISPLAY_INIT, STATE_ERROR, CONDITION_ERROR, validate_error_entry},
    
    // Idle transitions
    {STATE_IDLE, STATE_SYNCING, CONDITION_SYNC_RECEIVED, validate_sync_request},
    {STATE_IDLE, STATE_ERROR, CONDITION_ERROR, validate_error_entry},
    
    // Syncing transitions
    {STATE_SYNCING, STATE_READY, CONDITION_SYNC_VALID, NULL},
    {STATE_SYNCING, STATE_SYNCING, CONDITION_RETRY, NULL},
    {STATE_SYNCING, STATE_ERROR, CONDITION_ERROR, validate_error_entry},
    
    // Ready transitions
    {STATE_READY, STATE_COMMAND_PROCESSING, CONDITION_COMMAND_VALID, validate_command},
    {STATE_READY, STATE_DATA_TRANSFER, CONDITION_TRANSFER_START, validate_transfer},
    {STATE_READY, STATE_IDLE, CONDITION_RESET, NULL},
    {STATE_READY, STATE_ERROR, CONDITION_ERROR, validate_error_entry},
    
    // Command processing transitions
    {STATE_COMMAND_PROCESSING, STATE_READY, CONDITION_TRANSFER_COMPLETE, NULL},
    {STATE_COMMAND_PROCESSING, STATE_ERROR, CONDITION_ERROR, validate_error_entry},
    
    // Data transfer transitions
    {STATE_DATA_TRANSFER, STATE_DATA_TRANSFER, CONDITION_TRANSFER_START, validate_transfer},
    {STATE_DATA_TRANSFER, STATE_READY, CONDITION_TRANSFER_COMPLETE, NULL},
    {STATE_DATA_TRANSFER, STATE_ERROR, CONDITION_ERROR, validate_error_entry},
    
    // Error state transitions
    {STATE_ERROR, STATE_IDLE, CONDITION_RESET, NULL},
    {STATE_ERROR, STATE_SYNCING, CONDITION_RETRY, validate_error_recovery},
    
    // Sentinel
    {STATE_ERROR, STATE_ERROR, CONDITION_NONE, NULL}
};

#define TRANSITION_COUNT (sizeof(VALID_TRANSITIONS) / sizeof(VALID_TRANSITIONS[0]))

// Validation function implementations
static bool validate_hardware_init(void) {
    // TODO: Implement hardware initialization check
    return true;
}

static bool validate_display_init(void) {
    // TODO: Implement display initialization check
    return true;
}

static bool validate_sync_request(void) {
    // TODO: Implement sync request validation
    return true;
}

static bool validate_command(void) {
    // TODO: Implement command validation
    return true;
}

static bool validate_transfer(void) {
    // TODO: Implement transfer validation
    return true;
}

static bool validate_error_entry(void) {
    // Always valid to enter error state
    return true;
}

static bool validate_error_recovery(void) {
    // Check if recovery is possible
    return state_context_can_retry();
}

bool transition_is_valid(const StateContext *ctx, SystemState next_state, StateCondition condition) {
    if (!ctx) {
        return false;
    }

    // Find matching transition
    const StateTransition *transition = NULL;
    for (size_t i = 0; i < TRANSITION_COUNT; i++) {
        if (VALID_TRANSITIONS[i].from_state == ctx->current_state &&
            VALID_TRANSITIONS[i].to_state == next_state &&
            VALID_TRANSITIONS[i].condition == condition) {
            transition = &VALID_TRANSITIONS[i];
            break;
        }
    }

    if (!transition) {
        char context[256];
        snprintf(context, sizeof(context),
                "Invalid transition: %s -> %s (%s)",
                state_to_string(ctx->current_state),
                state_to_string(next_state),
                condition_to_string(condition));
        logging_write_with_context("State", "Invalid transition", context);
        return false;
    }

    // Run validator if present
    if (transition->validator && !transition->validator()) {
        char context[256];
        snprintf(context, sizeof(context),
                "Validation failed: %s -> %s",
                state_to_string(ctx->current_state),
                state_to_string(next_state));
        logging_write_with_context("State", "Transition validation failed", context);
        return false;
    }

    return true;
}

bool transition_can_recover(const StateContext *ctx) {
    if (!ctx || ctx->current_state != STATE_ERROR) {
        return false;
    }

    // Check retry count
    return state_context_can_retry();
}

bool transition_entry(StateContext *ctx) {
    if (!ctx) {
        return false;
    }

    // Update timestamp
    ctx->last_update = deskthang_time_get_ms();

    // Reset retry count if not entering error state
    if (ctx->current_state != STATE_ERROR) {
        state_context_reset_retry();
    }

    // Execute entry actions
    const StateActions *actions = &STATE_ACTIONS[ctx->current_state];
    if (actions && actions->on_entry) {
        actions->on_entry();
    }

    return true;
}

bool transition_exit(StateContext *ctx) {
    if (!ctx) {
        return false;
    }

    // Execute exit actions
    const StateActions *actions = &STATE_ACTIONS[ctx->current_state];
    if (actions && actions->on_exit) {
        actions->on_exit();
    }

    return true;
}

// State action execution implementations
bool transition_execute_entry_actions(SystemState state) {
    const StateActions *actions = &STATE_ACTIONS[state];
    if (actions && actions->on_entry) {
        actions->on_entry();
        return true;
    }
    return false;
}

bool transition_execute_exit_actions(SystemState state) {
    const StateActions *actions = &STATE_ACTIONS[state];
    if (actions && actions->on_exit) {
        actions->on_exit();
        return true;
    }
    return false;
}

bool transition_execute_error_handler(SystemState state, void *error_context) {
    const StateActions *actions = &STATE_ACTIONS[state];
    if (actions && actions->on_error) {
        actions->on_error(error_context);
        return true;
    }
    return false;
}
