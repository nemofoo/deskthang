#include "context.h"
#include <string.h>

// Global state context
static StateContext g_state_context;

// Initialize state context
bool state_context_init(void) {
    memset(&g_state_context, 0, sizeof(StateContext));
    return true;
}

// Reset state context to initial values
void state_context_reset(void) {
    g_state_context.retry_count = 0;
    g_state_context.error_count = 0;
    if (g_state_context.state_data) {
        state_context_clear_data();
    }
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
    if (g_state_context.retry_count >= 8) { // Max 8 retries as per protocol spec
        return false;
    }
    g_state_context.retry_count++;
    return true;
}

void state_context_reset_retry(void) {
    g_state_context.retry_count = 0;
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
    return current_time - g_state_context.transition_time;
}

void state_context_update_transition_time(void) {
    g_state_context.transition_time = get_system_time();
}
