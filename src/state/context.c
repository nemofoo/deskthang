#include "context.h"
#include "../protocol/protocol.h"
#include <string.h>

// Global state context
static StateContext g_state_context;

// Initialize state context
bool state_context_init(void) {
    memset(&g_state_context, 0, sizeof(StateContext));
    g_state_context.current = STATE_HARDWARE_INIT;
    g_state_context.previous = STATE_HARDWARE_INIT;
    g_state_context.last_condition = CONDITION_NONE;
    g_state_context.entry_time = get_system_time();
    g_state_context.last_update = g_state_context.entry_time;
    g_state_context.can_retry = true;
    return true;
}

// Reset state context to initial values
void state_context_reset(void) {
    // Save current state info
    SystemState current = g_state_context.current;
    SystemState previous = g_state_context.previous;
    
    // Clear context
    memset(&g_state_context, 0, sizeof(StateContext));
    
    // Restore state info
    g_state_context.current = current;
    g_state_context.previous = previous;
    g_state_context.entry_time = get_system_time();
    g_state_context.last_update = g_state_context.entry_time;
    g_state_context.can_retry = true;
}

// Get pointer to current state context
StateContext *state_context_get(void) {
    return &g_state_context;
}

// Set state-specific data
bool state_context_set_data(void *data) {
    if (g_state_context.state_data) {
        state_context_clear_data();
    }
    g_state_context.state_data = data;
    return true;
}

// Get state-specific data
void *state_context_get_data(void) {
    return g_state_context.state_data;
}

// Clear state-specific data
void state_context_clear_data(void) {
    if (g_state_context.state_data) {
        free(g_state_context.state_data);
        g_state_context.state_data = NULL;
    }
}

// Retry management
bool state_context_increment_retry(void) {
    // Check if we've exceeded maximum retries
    if (g_state_context.retry_count >= MAX_RETRIES) {
        return false;
    }
    g_state_context.retry_count++;
    g_state_context.can_retry = (g_state_context.retry_count < MAX_RETRIES);
    return true;
}

void state_context_reset_retry(void) {
    g_state_context.retry_count = 0;
    g_state_context.can_retry = true;
}

uint32_t state_context_get_retry_count(void) {
    return g_state_context.retry_count;
}

// Error tracking
void state_context_increment_error(void) {
    g_state_context.error_count++;
}

void state_context_reset_error(void) {
    g_state_context.error_count = 0;
}

uint32_t state_context_get_error_count(void) {
    return g_state_context.error_count;
}

// Timing functions
extern uint32_t get_system_time(void); // Implemented elsewhere

uint32_t state_context_get_time_in_state(void) {
    uint32_t current_time = get_system_time();
    return current_time - g_state_context.entry_time;
}

void state_context_update_transition_time(void) {
    g_state_context.last_update = get_system_time();
}

void context_init(StateContext *ctx) {
    if (!ctx) return;
    
    memset(ctx, 0, sizeof(StateContext));
    ctx->current = STATE_HARDWARE_INIT;
    ctx->previous = STATE_HARDWARE_INIT;
    ctx->last_condition = CONDITION_NONE;
    ctx->entry_time = get_system_time();
    ctx->last_update = ctx->entry_time;
    ctx->can_retry = true;
}

void context_reset(StateContext *ctx) {
    if (!ctx) return;
    
    // Save current state info
    SystemState current = ctx->current;
    SystemState previous = ctx->previous;
    
    // Clear context
    memset(ctx, 0, sizeof(StateContext));
    
    // Restore state info
    ctx->current = current;
    ctx->previous = previous;
    ctx->entry_time = get_system_time();
    ctx->last_update = ctx->entry_time;
    ctx->can_retry = true;
}

void context_update_timing(StateContext *ctx) {
    if (!ctx) return;
    ctx->last_update = get_system_time();
}

void context_set_state_data(StateContext *ctx, void *data) {
    if (!ctx) return;
    context_clear_state_data(ctx);
    ctx->state_data = data;
}

void *context_get_state_data(StateContext *ctx) {
    return ctx ? ctx->state_data : NULL;
}

void context_clear_state_data(StateContext *ctx) {
    if (!ctx) return;
    if (ctx->state_data) {
        free(ctx->state_data);
        ctx->state_data = NULL;
    }
}

bool context_can_retry(StateContext *ctx) {
    if (!ctx) return false;
    return ctx->can_retry && (ctx->retry_count < MAX_RETRIES);
}

void context_increment_retry(StateContext *ctx) {
    if (!ctx) return;
    ctx->retry_count++;
    ctx->can_retry = (ctx->retry_count < MAX_RETRIES);
}

void context_reset_retry(StateContext *ctx) {
    if (!ctx) return;
    ctx->retry_count = 0;
    ctx->can_retry = true;
}
