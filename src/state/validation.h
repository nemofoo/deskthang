#ifndef DESKTHANG_VALIDATION_H
#define DESKTHANG_VALIDATION_H

#include <stdbool.h>
#include <stdint.h>
#include "state.h"
#include "context.h"

// State validation flags
typedef enum {
    STATE_VALID_NONE = 0x00,
    STATE_VALID_RESOURCES = 0x01,    // Required resources are available
    STATE_VALID_TIMING = 0x02,       // Timing constraints met
    STATE_VALID_HISTORY = 0x04,      // State history is valid
    STATE_VALID_ENTRY = 0x08,        // Entry conditions met
    STATE_VALID_ALL = 0x0F           // All validation checks passed
} StateValidFlags;

// Resource requirements for each state
typedef struct {
    uint32_t min_memory;             // Minimum free memory required
    uint32_t max_duration;           // Maximum allowed duration in state
    uint32_t min_duration;           // Minimum required duration in state
    uint32_t max_retries;            // Maximum allowed retries
    uint32_t required_flags;         // Required validation flags
} StateRequirements;

// State history entry
typedef struct {
    SystemState state;               // The state that was entered
    uint32_t entry_time;            // When the state was entered
    uint32_t exit_time;             // When the state was exited
    StateCondition entry_condition;  // Condition that triggered entry
    StateCondition exit_condition;   // Condition that triggered exit
} StateHistoryEntry;

// State validation functions
bool validate_state_entry(const StateContext *ctx, SystemState state);
bool validate_state_resources(const StateContext *ctx, SystemState state);
bool validate_state_timing(const StateContext *ctx, SystemState state);
bool validate_state_history(const StateContext *ctx, SystemState state);

// Combined validation
bool validate_state_all(const StateContext *ctx, SystemState state);

// History management
void state_history_add(SystemState state, StateCondition entry_condition);
void state_history_update_exit(StateCondition exit_condition);
const StateHistoryEntry *state_history_get_last(void);
bool state_history_validate_sequence(void);

// Resource management
void state_update_resource_usage(void);

// Timing management
bool state_check_timing_constraints(const StateContext *ctx, SystemState state);
uint32_t state_get_remaining_time(const StateContext *ctx, SystemState state);

// State validation
bool state_validate_entry_conditions(SystemState state);
bool state_validate_timing(SystemState state);
bool state_validate_history(SystemState state);

#endif // DESKTHANG_VALIDATION_H 