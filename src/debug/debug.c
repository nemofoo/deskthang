#include "debug.h"
#include "../error/logging.h"
#include "../system/time.h"
#include <string.h>
#include <stdio.h>

// Static debug state
static struct {
    bool enabled;
    StateDebugStats state_stats;
    ResourceDebugStats resource_stats;
    PerformanceStats perf_stats;
    struct {
        uint32_t start_time;
        char operation[32];
    } current_operation;
    uint32_t state_durations[STATE_COUNT];
    uint32_t state_transitions[STATE_COUNT];
} debug_state = {0};

void debug_init(void) {
    memset(&debug_state, 0, sizeof(debug_state));
    debug_state.perf_stats.shortest_operation_ms = UINT32_MAX;
    debug_state.enabled = true;
}

void debug_enable(void) {
    debug_state.enabled = true;
    logging_write("Debug", "Debug monitoring enabled");
}

void debug_disable(void) {
    debug_state.enabled = false;
    logging_write("Debug", "Debug monitoring disabled");
}

bool debug_is_enabled(void) {
    return debug_state.enabled;
}

void debug_log_transition(SystemState from, SystemState to, StateCondition condition, bool success) {
    if (!debug_state.enabled) return;

    debug_state.state_stats.total_transitions++;
    if (!success) {
        debug_state.state_stats.failed_transitions++;
        return;
    }

    // Update state statistics
    debug_state.state_transitions[to]++;
    if (debug_state.state_transitions[to] > debug_state.state_transitions[debug_state.state_stats.most_frequent_state]) {
        debug_state.state_stats.most_frequent_state = to;
    }

    // Log transition details
    char message[128];
    snprintf(message, sizeof(message), 
            "State transition: %s -> %s (Condition: %s)", 
            state_to_string(from), 
            state_to_string(to),
            condition_to_string(condition));
    logging_write("Debug", message);
}

void debug_log_validation_failure(SystemState from, SystemState to, const char* reason) {
    if (!debug_state.enabled) return;

    debug_state.state_stats.validation_failures++;
    
    char message[128];
    snprintf(message, sizeof(message), 
            "Validation failed: %s -> %s (%s)", 
            state_to_string(from), 
            state_to_string(to),
            reason);
    logging_write("Debug", message);
}

StateDebugStats* debug_get_state_stats(void) {
    return &debug_state.state_stats;
}

void debug_log_buffer_usage(size_t current_usage, size_t max_size) {
    if (!debug_state.enabled) return;

    if (current_usage > debug_state.resource_stats.peak_buffer_usage) {
        debug_state.resource_stats.peak_buffer_usage = current_usage;
    }

    if (current_usage == max_size) {
        debug_state.resource_stats.buffer_full_count++;
    }

    debug_state.resource_stats.bytes_processed += current_usage;
}

void debug_log_overflow(void) {
    if (!debug_state.enabled) return;

    debug_state.resource_stats.total_overflows++;
    debug_state.resource_stats.last_overflow_time = deskthang_time_get_ms();
    
    char message[64];
    snprintf(message, sizeof(message), 
            "Buffer overflow detected (Total: %lu)", 
            debug_state.resource_stats.total_overflows);
    logging_write("Debug", message);
}

ResourceDebugStats* debug_get_resource_stats(void) {
    return &debug_state.resource_stats;
}

void debug_log_operation_start(const char* operation) {
    if (!debug_state.enabled) return;

    debug_state.current_operation.start_time = deskthang_time_get_ms();
    strncpy(debug_state.current_operation.operation, operation, sizeof(debug_state.current_operation.operation) - 1);
    debug_state.current_operation.operation[sizeof(debug_state.current_operation.operation) - 1] = '\0';
}

void debug_log_operation_end(const char* operation) {
    if (!debug_state.enabled) return;

    if (strcmp(operation, debug_state.current_operation.operation) != 0) {
        return; // Mismatched operation
    }

    uint32_t duration = deskthang_time_get_ms() - debug_state.current_operation.start_time;
    
    // Update statistics
    if (duration > debug_state.perf_stats.longest_operation_ms) {
        debug_state.perf_stats.longest_operation_ms = duration;
    }
    if (duration < debug_state.perf_stats.shortest_operation_ms) {
        debug_state.perf_stats.shortest_operation_ms = duration;
    }

    char message[128];
    snprintf(message, sizeof(message), 
            "Operation '%s' completed in %lu ms", 
            operation, duration);
    logging_write("Debug", message);
}

void debug_log_retry(const char* operation) {
    if (!debug_state.enabled) return;

    debug_state.perf_stats.total_retries++;
    
    char message[64];
    snprintf(message, sizeof(message), 
            "Retrying operation '%s' (Total retries: %lu)", 
            operation, debug_state.perf_stats.total_retries);
    logging_write("Debug", message);
}

PerformanceStats* debug_get_performance_stats(void) {
    return &debug_state.perf_stats;
}

void debug_print_summary(void) {
    if (!debug_state.enabled) return;

    // State statistics
    logging_write("Debug", "=== State Machine Statistics ===");
    char state_stats[128];
    snprintf(state_stats, sizeof(state_stats),
            "Transitions: %lu, Failed: %lu, Validation Failures: %lu",
            debug_state.state_stats.total_transitions,
            debug_state.state_stats.failed_transitions,
            debug_state.state_stats.validation_failures);
    logging_write("Debug", state_stats);

    // Resource statistics
    logging_write("Debug", "=== Resource Statistics ===");
    char resource_stats[128];
    snprintf(resource_stats, sizeof(resource_stats),
            "Peak Buffer: %lu, Overflows: %lu, Buffer Full Events: %lu",
            debug_state.resource_stats.peak_buffer_usage,
            debug_state.resource_stats.total_overflows,
            debug_state.resource_stats.buffer_full_count);
    logging_write("Debug", resource_stats);

    // Performance statistics
    logging_write("Debug", "=== Performance Statistics ===");
    char perf_stats[128];
    snprintf(perf_stats, sizeof(perf_stats),
            "Longest Op: %lu ms, Shortest Op: %lu ms, Retries: %lu",
            debug_state.perf_stats.longest_operation_ms,
            debug_state.perf_stats.shortest_operation_ms,
            debug_state.perf_stats.total_retries);
    logging_write("Debug", perf_stats);
}

void debug_reset_stats(void) {
    memset(&debug_state.state_stats, 0, sizeof(StateDebugStats));
    memset(&debug_state.resource_stats, 0, sizeof(ResourceDebugStats));
    memset(&debug_state.perf_stats, 0, sizeof(PerformanceStats));
    memset(debug_state.state_durations, 0, sizeof(debug_state.state_durations));
    memset(debug_state.state_transitions, 0, sizeof(debug_state.state_transitions));
    debug_state.perf_stats.shortest_operation_ms = UINT32_MAX;
    logging_write("Debug", "Debug statistics reset");
} 