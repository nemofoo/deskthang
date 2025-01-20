#include "error.h"
#include "../system/time.h"
#include <string.h>
#include <stdio.h>

// Global error state
static ErrorDetails g_last_error = {0};
static bool g_initialized = false;

void error_init(void) {
    memset(&g_last_error, 0, sizeof(ErrorDetails));
    g_initialized = true;
}

void error_report(ErrorType type, ErrorSeverity severity, uint32_t code, const char *message) {
    if (!g_initialized) {
        return;
    }

    // Validate error code range
    if (!error_code_in_range(type, code)) {
        // If invalid code provided, report a system error instead
        g_last_error.type = ERROR_TYPE_SYSTEM;
        g_last_error.severity = ERROR_SEVERITY_ERROR;
        g_last_error.code = 6001; // System error: Invalid error code
        g_last_error.timestamp = get_system_time();
        snprintf(g_last_error.message, sizeof(g_last_error.message),
                "Invalid error code %u for type %s", code, error_type_to_string(type));
        g_last_error.recoverable = true;
        return;
    }

    // Store error details
    g_last_error.type = type;
    g_last_error.severity = severity;
    g_last_error.code = code;
    g_last_error.timestamp = get_system_time();
    
    // Copy message with bounds checking
    strncpy(g_last_error.message, message ? message : "Unknown error", sizeof(g_last_error.message) - 1);
    g_last_error.message[sizeof(g_last_error.message) - 1] = '\0';
    
    // Set recoverable flag based on severity
    g_last_error.recoverable = (severity != ERROR_SEVERITY_FATAL);
}

ErrorDetails *error_get_last(void) {
    return &g_last_error;
}

bool error_is_recoverable(const ErrorDetails *error) {
    if (!error) {
        return false;
    }
    return error->recoverable;
}

bool error_code_in_range(ErrorType type, uint32_t code) {
    switch (type) {
        case ERROR_TYPE_HARDWARE:
            return (code >= 1000 && code <= 1999);
        case ERROR_TYPE_PROTOCOL:
            return (code >= 2000 && code <= 2999);
        case ERROR_TYPE_STATE:
            return (code >= 3000 && code <= 3999);
        case ERROR_TYPE_COMMAND:
            return (code >= 4000 && code <= 4999);
        case ERROR_TYPE_TRANSFER:
            return (code >= 5000 && code <= 5999);
        case ERROR_TYPE_SYSTEM:
            return (code >= 6000 && code <= 6999);
        default:
            return false;
    }
}

const char *error_type_to_string(ErrorType type) {
    switch (type) {
        case ERROR_TYPE_NONE:
            return "NONE";
        case ERROR_TYPE_HARDWARE:
            return "HARDWARE";
        case ERROR_TYPE_PROTOCOL:
            return "PROTOCOL";
        case ERROR_TYPE_STATE:
            return "STATE";
        case ERROR_TYPE_COMMAND:
            return "COMMAND";
        case ERROR_TYPE_TRANSFER:
            return "TRANSFER";
        case ERROR_TYPE_SYSTEM:
            return "SYSTEM";
        default:
            return "UNKNOWN";
    }
}

const char *error_severity_to_string(ErrorSeverity severity) {
    switch (severity) {
        case ERROR_SEVERITY_INFO:
            return "INFO";
        case ERROR_SEVERITY_WARNING:
            return "WARNING";
        case ERROR_SEVERITY_ERROR:
            return "ERROR";
        case ERROR_SEVERITY_FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}
