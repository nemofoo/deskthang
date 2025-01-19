#ifndef DESKTHANG_STATE_CONTEXT_H
#define DESKTHANG_STATE_CONTEXT_H

#include "state.h"
#include <stdint.h>

// State context information
typedef struct {
    SystemState current;         // Current state
    SystemState previous;        // Previous state
    uint32_t transition_time;    // Time of last transition
    uint32_t retry_count;        // Number of retries in current state
    uint32_t error_count;        // Number of errors in current state
    void *state_data;           // State-specific data
} StateContext;

// Context management functions
bool state_context_init(void);
void state_context_reset(void);
StateContext *state_context_get(void);

// State data management
bool state_context_set_data(void *data);
void *state_context_get_data(void);
void state_context_clear_data(void);

// Retry management
bool state_context_increment_retry(void);
void state_context_reset_retry(void);
uint32_t state_context_get_retry_count(void);

// Error tracking
void state_context_increment_error(void);
void state_context_reset_error(void);
uint32_t state_context_get_error_count(void);

// Timing functions
uint32_t state_context_get_time_in_state(void);
void state_context_update_transition_time(void);

#endif // DESKTHANG_STATE_CONTEXT_H
