#include "transition.h"
#include "../error/logging.h"
#include "../system/time.h"
#include <stdio.h>

// Add at top of file after includes
extern const StateActions STATE_ACTIONS[];

// Valid state transitions table
static const TransitionRule VALID_TRANSITIONS[] = {
    // Hardware init transitions
    {STATE_HARDWARE_INIT, STATE_DISPLAY_INIT, CONDITION_HARDWARE_READY},
    {STATE_HARDWARE_INIT, STATE_ERROR, CONDITION_ERROR},
    
    // Display init transitions
    {STATE_DISPLAY_INIT, STATE_IDLE, CONDITION_DISPLAY_READY},
    {STATE_DISPLAY_INIT, STATE_ERROR, CONDITION_ERROR},
    
    // Idle transitions
    {STATE_IDLE, STATE_SYNCING, CONDITION_SYNC_RECEIVED},
    {STATE_IDLE, STATE_ERROR, CONDITION_ERROR},
    
    // Syncing transitions
    {STATE_SYNCING, STATE_READY, CONDITION_SYNC_VALID},
    {STATE_SYNCING, STATE_SYNCING, CONDITION_RETRY},
    {STATE_SYNCING, STATE_ERROR, CONDITION_ERROR},
    
    // Ready transitions
    {STATE_READY, STATE_COMMAND_PROCESSING, CONDITION_COMMAND_VALID},
    {STATE_READY, STATE_DATA_TRANSFER, CONDITION_TRANSFER_START},
    {STATE_READY, STATE_IDLE, CONDITION_RESET},
    {STATE_READY, STATE_ERROR, CONDITION_ERROR},
    
    // Command processing transitions
    {STATE_COMMAND_PROCESSING, STATE_READY, CONDITION_TRANSFER_COMPLETE},
    {STATE_COMMAND_PROCESSING, STATE_ERROR, CONDITION_ERROR},
    
    // Data transfer transitions
    {STATE_DATA_TRANSFER, STATE_DATA_TRANSFER, CONDITION_TRANSFER_START},
    {STATE_DATA_TRANSFER, STATE_READY, CONDITION_TRANSFER_COMPLETE},
    {STATE_DATA_TRANSFER, STATE_ERROR, CONDITION_ERROR},
    
    // Error state transitions
    {STATE_ERROR, STATE_IDLE, CONDITION_RESET},
    {STATE_ERROR, STATE_SYNCING, CONDITION_RETRY},
    
    // Sentinel
    {STATE_ERROR, STATE_ERROR, CONDITION_NONE}
};

// Add after transitions table
#define TRANSITION_RULE_COUNT (sizeof(VALID_TRANSITIONS) / sizeof(VALID_TRANSITIONS[0]))

bool transition_is_valid(const StateContext *ctx, SystemState next_state, StateCondition condition) {
    if (!ctx) {
        return false;
    }

    // Find matching transition rule
    for (size_t i = 0; i < TRANSITION_RULE_COUNT; i++) {
        const TransitionRule *rule = &VALID_TRANSITIONS[i];
        if (rule->from_state == ctx->current_state &&
            rule->to_state == next_state &&
            rule->condition == condition) {
            return true;
        }
    }

    char context[256];
    snprintf(context, sizeof(context),
             "Invalid transition: %s -> %s (%s)",
             state_to_string(ctx->current_state),
             state_to_string(next_state),
             condition_to_string(condition));
    logging_write_with_context("State", "Invalid transition", context);

    return false;
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

    // Update timing
    ctx->last_update = get_system_time();

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
