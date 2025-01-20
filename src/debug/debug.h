#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // For size_t
#include "../state/state.h"
#include "../common/deskthang_constants.h"

// Debug statistics structure for state transitions
typedef struct {
    uint32_t total_transitions;
    uint32_t failed_transitions;
    uint32_t validation_failures;
    uint32_t avg_transition_time_ms;
    SystemState most_frequent_state;
    uint32_t error_count;
} StateDebugStats;

// Debug statistics for resource usage
typedef struct {
    uint32_t peak_buffer_usage;
    uint32_t total_overflows;
    uint32_t last_overflow_time;
    uint32_t buffer_full_count;
    uint32_t bytes_processed;
} ResourceDebugStats;

// Performance metrics
typedef struct {
    uint32_t longest_operation_ms;
    uint32_t shortest_operation_ms;
    uint32_t total_retries;
    uint32_t operation_timeouts;
} PerformanceStats;

// Debug control
void debug_init(void);
void debug_enable(void);
void debug_disable(void);
bool debug_is_enabled(void);

// State transition debugging
void debug_log_transition(SystemState from, SystemState to, StateCondition condition, bool success);
void debug_log_validation_failure(SystemState from, SystemState to, const char* reason);
StateDebugStats* debug_get_state_stats(void);

// Resource usage debugging
void debug_log_buffer_usage(size_t current_usage, size_t max_size);
void debug_log_overflow(void);
ResourceDebugStats* debug_get_resource_stats(void);

// Performance monitoring
void debug_log_operation_start(const char* operation);
void debug_log_operation_end(const char* operation);
void debug_log_retry(const char* operation);
PerformanceStats* debug_get_performance_stats(void);

// Debug reporting
void debug_print_summary(void);
void debug_reset_stats(void);

#endif // DEBUG_H 