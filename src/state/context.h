#ifndef DESKTHANG_STATE_CONTEXT_H
#define DESKTHANG_STATE_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include "state.h"

// State context structure
typedef struct {
    // Current state tracking
    SystemState current;       // Current state
    SystemState previous;      // Previous state
    StateCondition last_condition; // Last transition condition
    
    // Timing
    uint32_t entry_time;      // When we entered current state
    uint32_t last_update;     // Last state update time
    
    // Error tracking
    uint8_t retry_count;      // Number of retries in current state
    bool can_retry;           // Whether retry is allowed
    
    // State-specific data
    void *state_data;         // Optional state-specific context
} StateContext;

// Context management
void context_init(StateContext *ctx);
void context_reset(StateContext *ctx);
void context_update_timing(StateContext *ctx);

// State data management
void context_set_state_data(StateContext *ctx, void *data);
void *context_get_state_data(StateContext *ctx);
void context_clear_state_data(StateContext *ctx);

// Retry management
bool context_can_retry(StateContext *ctx);
void context_increment_retry(StateContext *ctx);
void context_reset_retry(StateContext *ctx);

#endif // DESKTHANG_STATE_CONTEXT_H
