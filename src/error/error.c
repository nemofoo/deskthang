#include "error.h"
#include <string.h>
#include <stdio.h>

// Maximum number of errors to keep in history
#define MAX_ERROR_HISTORY 32

// Global error tracking
static ErrorDetails g_last_error;
static ErrorHistoryEntry g_error_history[MAX_ERROR_HISTORY];
static uint32_t g_history_count = 0;
static ErrorStats g_error_stats;

// Initialize error handling
bool error_init(void) {
    memset(&g_last_error, 0, sizeof(ErrorDetails));
    memset(g_error_history, 0, sizeof(g_error_history));
    memset(&g_error_stats, 0, sizeof(ErrorStats));
    g_history_count = 0;
    return true;
}

// Reset error state
void error_reset(void) {
    memset(&g_last_error, 0, sizeof(ErrorDetails));
    memset(g_error_history, 0, sizeof(g_error_history));
    memset(&g_error_stats, 0, sizeof(ErrorStats));
    g_history_count = 0;
}

// Report a new error
void error_report(ErrorCategory category, ErrorSeverity severity, uint32_t code, const char *message) {
    error_report_with_context(category, severity, code, message, NULL);
}

// Report error with context
void error_report_with_context(ErrorCategory category, ErrorSeverity severity, uint32_t code, 
                             const char *message, const char *context) {
    // Update last error
    g_last_error.category = category;
    g_last_error.severity = severity;
    g_last_error.code = code;
    g_last_error.timestamp = get_system_time();
    g_last_error.state = state_machine_get_current();
    
    if (message) {
        strncpy(g_last_error.message, message, sizeof(g_last_error.message) - 1);
    } else {
        g_last_error.message[0] = '\0';
    }
    
    if (context) {
        strncpy(g_last_error.context, context, sizeof(g_last_error.context) - 1);
    } else {
        g_last_error.context[0] = '\0';
    }
    
    // Determine if error is recoverable
    g_last_error.recoverable = (severity != ERROR_SEVERITY_FATAL);
    
    // Update error history
    bool found = false;
    for (uint32_t i = 0; i < g_history_count; i++) {
        if (g_error_history[i].error.code == code) {
            // Update existing entry
            g_error_history[i].frequency++;
            g_error_history[i].last_seen = g_last_error.timestamp;
            found = true;
            break;
        }
    }
    
    if (!found && g_history_count < MAX_ERROR_HISTORY) {
        // Add new entry
        uint32_t idx = g_history_count++;
        memcpy(&g_error_history[idx].error, &g_last_error, sizeof(ErrorDetails));
        g_error_history[idx].frequency = 1;
        g_error_history[idx].first_seen = g_last_error.timestamp;
        g_error_history[idx].last_seen = g_last_error.timestamp;
    }
    
    // Update statistics
    g_error_stats.total_errors++;
    if (g_last_error.recoverable) {
        g_error_stats.recoverable++;
    } else {
        g_error_stats.unrecoverable++;
    }
}

// Error query functions
ErrorDetails *error_get_last(void) {
    return &g_last_error;
}

ErrorDetails *error_get_by_code(uint32_t code) {
    for (uint32_t i = 0; i < g_history_count; i++) {
        if (g_error_history[i].error.code == code) {
            return &g_error_history[i].error;
        }
    }
    return NULL;
}

ErrorHistoryEntry *error_get_history(void) {
    return g_error_history;
}

uint32_t error_get_history_count(void) {
    return g_history_count;
}

// Error statistics
ErrorStats *error_get_stats(void) {
    return &g_error_stats;
}

void error_update_stats(bool recovered) {
    if (recovered) {
        g_error_stats.recoveries++;
    }
    g_error_stats.retries++;
}

void error_reset_stats(void) {
    memset(&g_error_stats, 0, sizeof(ErrorStats));
}

// Error classification
bool error_is_recoverable(const ErrorDetails *error) {
    if (!error) {
        return false;
    }
    return error->recoverable;
}

bool error_requires_reset(const ErrorDetails *error) {
    if (!error) {
        return false;
    }
    return error->severity == ERROR_SEVERITY_FATAL;
}

ErrorSeverity error_get_severity(uint32_t code) {
    ErrorDetails *error = error_get_by_code(code);
    if (!error) {
        return ERROR_SEVERITY_ERROR; // Default to error severity
    }
    return error->severity;
}

// Debug support
void error_print_last(void) {
    printf("Last Error:\n");
    printf("  Category: %s\n", error_category_to_string(g_last_error.category));
    printf("  Severity: %s\n", error_severity_to_string(g_last_error.severity));
    printf("  Code: %u\n", g_last_error.code);
    printf("  State: %d\n", g_last_error.state);
    printf("  Recoverable: %s\n", g_last_error.recoverable ? "Yes" : "No");
    printf("  Message: %s\n", g_last_error.message);
    if (g_last_error.context[0]) {
        printf("  Context: %s\n", g_last_error.context);
    }
}

void error_print_history(void) {
    printf("Error History (%u entries):\n", g_history_count);
    for (uint32_t i = 0; i < g_history_count; i++) {
        printf("\nError %u:\n", i + 1);
        printf("  Code: %u\n", g_error_history[i].error.code);
        printf("  Category: %s\n", error_category_to_string(g_error_history[i].error.category));
        printf("  Severity: %s\n", error_severity_to_string(g_error_history[i].error.severity));
        printf("  Frequency: %u\n", g_error_history[i].frequency);
        printf("  First Seen: %u\n", g_error_history[i].first_seen);
        printf("  Last Seen: %u\n", g_error_history[i].last_seen);
        printf("  Message: %s\n", g_error_history[i].error.message);
    }
}

void error_print_stats(void) {
    printf("Error Statistics:\n");
    printf("  Total Errors: %u\n", g_error_stats.total_errors);
    printf("  Recoverable: %u\n", g_error_stats.recoverable);
    printf("  Unrecoverable: %u\n", g_error_stats.unrecoverable);
    printf("  Retries: %u\n", g_error_stats.retries);
    printf("  Successful Recoveries: %u\n", g_error_stats.recoveries);
}

const char *error_severity_to_string(ErrorSeverity severity) {
    switch (severity) {
        case ERROR_SEVERITY_INFO:    return "INFO";
        case ERROR_SEVERITY_WARNING: return "WARNING";
        case ERROR_SEVERITY_ERROR:   return "ERROR";
        case ERROR_SEVERITY_FATAL:   return "FATAL";
        default:                     return "UNKNOWN";
    }
}

const char *error_category_to_string(ErrorCategory category) {
    switch (category) {
        case ERROR_CAT_NONE:     return "NONE";
        case ERROR_CAT_HARDWARE: return "HARDWARE";
        case ERROR_CAT_PROTOCOL: return "PROTOCOL";
        case ERROR_CAT_STATE:    return "STATE";
        case ERROR_CAT_COMMAND:  return "COMMAND";
        case ERROR_CAT_TRANSFER: return "TRANSFER";
        case ERROR_CAT_SYSTEM:   return "SYSTEM";
        default:                 return "UNKNOWN";
    }
}
