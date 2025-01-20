#ifndef __DESKTHANG_CONTEXT_H
#define __DESKTHANG_CONTEXT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "state.h"

// State context structure
typedef struct {
    // Current state tracking
    SystemState current_state;
    SystemState previous_state;
    StateCondition last_condition;  // Added: Last transition condition
    
    // Timing
    uint32_t entry_time;      // When we entered current state
    uint32_t last_update;     // Added: Last state update time
    
    // Error tracking
    uint32_t error_count;     // Number of errors in current state
    uint32_t retry_count;     // Added: Number of retries attempted
    bool can_retry;           // Added: Whether retry is allowed
    
    // State-specific data
    void *state_data;         // Optional state-specific context
    size_t data_size;         // Size of state data
} StateContext;

// Context management functions
void state_context_init(void);
void state_context_set_state(SystemState state, StateCondition condition);  // Added condition
SystemState state_context_get_state(void);
SystemState state_context_get_previous(void);
uint32_t state_context_get_duration(void);
uint32_t state_context_get_last_update(void);  // Added

// State data management
bool state_context_set_data(void *data, size_t size);
void *state_context_get_data(void);
void state_context_clear_data(void);

// Error handling
void state_context_increment_error(void);
void state_context_reset_error(void);
uint32_t state_context_get_error_count(void);

// Retry management (Added)
bool state_context_can_retry(void);
void state_context_increment_retry(void);
void state_context_reset_retry(void);
uint32_t state_context_get_retry_count(void);

// Context validation
bool context_can_retry(StateContext *ctx);
bool context_is_valid(const StateContext *ctx);
void context_clear_state_data(StateContext *ctx);

#endif // __DESKTHANG_CONTEXT_H
