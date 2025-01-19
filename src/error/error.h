#ifndef DESKTHANG_ERROR_H
#define DESKTHANG_ERROR_H

#include <stdint.h>
#include <stdbool.h>
#include "../state/state.h"

// Error severity levels
typedef enum {
    ERROR_SEVERITY_INFO,     // Informational, no impact
    ERROR_SEVERITY_WARNING,  // Warning, operation can continue
    ERROR_SEVERITY_ERROR,    // Error, operation failed but recoverable
    ERROR_SEVERITY_FATAL     // Fatal error, system needs reset
} ErrorSeverity;

// Error categories
typedef enum {
    ERROR_TYPE_NONE,        // No specific type
    ERROR_TYPE_HARDWARE,    // Hardware-related errors
    ERROR_TYPE_PROTOCOL,    // Protocol-related errors
    ERROR_TYPE_STATE,       // State machine errors
    ERROR_TYPE_COMMAND,     // Command processing errors
    ERROR_TYPE_TRANSFER,    // Data transfer errors
    ERROR_TYPE_SYSTEM      // System-level errors
} ErrorType;

// Error details structure
typedef struct {
    ErrorType type;          // Error type (was category)
    ErrorSeverity severity;  // Error severity
    uint32_t code;          // Error code
    uint32_t timestamp;     // When error occurred
    SystemState state;      // State when error occurred
    char message[128];      // Error message
    char context[256];      // Additional context
    bool recoverable;       // Can be recovered from
} ErrorDetails;

// Error management functions
bool error_init(void);
void error_reset(void);

// Error reporting
void error_report(ErrorType type, ErrorSeverity severity, uint32_t code, const char *message);
void error_report_with_context(ErrorType type, ErrorSeverity severity, uint32_t code, 
                             const char *message, const char *context);

// Error query
ErrorDetails *error_get_last(void);

// Error classification
bool error_is_recoverable(const ErrorDetails *error);
bool error_requires_reset(const ErrorDetails *error);
ErrorSeverity error_get_severity(uint32_t code);

// Debug support
void error_print_last(void);
const char *error_severity_to_string(ErrorSeverity severity);
const char *error_type_to_string(ErrorType type);

#endif // DESKTHANG_ERROR_H
