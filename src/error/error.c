#include "error.h"
#include <string.h>
#include <stdio.h>
#include "../protocol/protocol.h"

// Global error tracking
static ErrorDetails g_last_error;

// Initialize error handling
bool error_init(void) {
    memset(&g_last_error, 0, sizeof(ErrorDetails));
    return true;
}

// Reset error state
void error_reset(void) {
    memset(&g_last_error, 0, sizeof(ErrorDetails));
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
}

// Error query functions
ErrorDetails *error_get_last(void) {
    return &g_last_error;
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
    if (code == g_last_error.code) {
        return g_last_error.severity;
    }
    return ERROR_SEVERITY_ERROR; // Default to error severity
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
