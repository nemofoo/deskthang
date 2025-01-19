#include "transition.h"
#include "../error/logging.h"

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

bool transition_is_valid(const StateContext *ctx, SystemState next, StateCondition condition) {
    if (!ctx) return false;
    
    // Any state can transition to ERROR with CONDITION_ERROR
    if (next == STATE_ERROR && condition == CONDITION_ERROR) {
        return true;
    }
    
    // Look up transition in table
    const TransitionRule *rule = VALID_TRANSITIONS;
    while (rule->condition != CONDITION_NONE) {
        if (rule->from_state == ctx->current && 
            rule->to_state == next && 
            rule->condition == condition) {
            return true;
        }
        rule++;
    }
    
    // Log invalid transition attempt
    char context[256];
    snprintf(context, sizeof(context),
        "From %s to %s (%s)",
        state_to_string(ctx->current),
        state_to_string(next),
        condition_to_string(condition));
    logging_write_with_context("Transition", "Invalid transition", context);
    
    return false;
}

bool transition_can_recover(const StateContext *ctx) {
    if (!ctx || ctx->current != STATE_ERROR) {
        return false;
    }
    
    // Can only recover if we have retries available
    return context_can_retry(ctx);
}

bool transition_entry(StateContext *ctx) {
    if (!ctx) return false;
    
    // Update timing
    context_update_timing(ctx);
    
    // Reset retry count on state entry unless entering ERROR state
    if (ctx->current != STATE_ERROR) {
        context_reset_retry(ctx);
    }
    
    // Execute state-specific entry actions
    const StateActions *actions = &STATE_ACTIONS[ctx->current];
    if (actions->on_entry) {
        actions->on_entry();
    }
    
    return true;
}

bool transition_exit(StateContext *ctx) {
    if (!ctx) return false;
    
    // Execute state-specific exit actions
    const StateActions *actions = &STATE_ACTIONS[ctx->current];
    if (actions->on_exit) {
        actions->on_exit();
    }
    
    // Clear any state-specific data
    context_clear_state_data(ctx);
    
    return true;
}
