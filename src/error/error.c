#include "error.h"
#include "../system/time.h"
#include "logging.h"
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
        g_last_error.timestamp = deskthang_time_get_ms();
        snprintf(g_last_error.message, sizeof(g_last_error.message),
                "Invalid error code %u for type %s", code, error_type_to_string(type));
        g_last_error.recoverable = true;
        return;
    }

    // Store error details
    g_last_error.type = type;
    g_last_error.severity = severity;
    g_last_error.code = code;
    g_last_error.timestamp = deskthang_time_get_ms();
    
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
            return (code >= ERROR_CODE_HARDWARE_START && code <= ERROR_CODE_HARDWARE_END);
        case ERROR_TYPE_PROTOCOL:
            return (code >= ERROR_CODE_PROTOCOL_START && code <= ERROR_CODE_PROTOCOL_END);
        case ERROR_TYPE_STATE:
            return (code >= ERROR_CODE_STATE_START && code <= ERROR_CODE_STATE_END);
        case ERROR_TYPE_COMMAND:
            return (code >= ERROR_CODE_COMMAND_START && code <= ERROR_CODE_COMMAND_END);
        case ERROR_TYPE_TRANSFER:
            return (code >= ERROR_CODE_TRANSFER_START && code <= ERROR_CODE_TRANSFER_END);
        case ERROR_TYPE_SYSTEM:
            return (code >= ERROR_CODE_SYSTEM_START && code <= ERROR_CODE_SYSTEM_END);
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

void error_print_last(void) {
    ErrorDetails *error = error_get_last();
    if (!error) {
        return;
    }
    logging_error(error);
}

bool error_requires_reset(const ErrorDetails *error) {
    if (!error) {
        return false;
    }
    return error->severity == ERROR_SEVERITY_FATAL;
}

void error_report_with_context(ErrorType type, const char* message, const char* context) {
    // For now, just combine message and context into a single message
    char combined_message[MAX_PACKET_SIZE/2];  // Match ErrorDetails.context size
    snprintf(combined_message, sizeof(combined_message), "%s [Context: %s]",
             message ? message : "", context ? context : "");
    
    // Use the standard error reporting with default severity
    error_report(type, ERROR_SEVERITY_ERROR, 0, combined_message);
}
