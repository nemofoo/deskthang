#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>
#include <stdbool.h>

// Error types as defined in protocol_architecture.md
typedef enum {
    ERROR_TYPE_NONE = 0,
    ERROR_TYPE_HARDWARE,    // 1000-1999: Hardware interface errors
    ERROR_TYPE_PROTOCOL,    // 2000-2999: Protocol handling errors  
    ERROR_TYPE_STATE,       // 3000-3999: State machine errors
    ERROR_TYPE_COMMAND,     // 4000-4999: Command processing errors
    ERROR_TYPE_TRANSFER,    // 5000-5999: Data transfer errors
    ERROR_TYPE_SYSTEM       // 6000-6999: System level errors
} ErrorType;

typedef enum {
    ERROR_SEVERITY_INFO,
    ERROR_SEVERITY_WARNING,
    ERROR_SEVERITY_ERROR,
    ERROR_SEVERITY_FATAL
} ErrorSeverity;

#include "../state/state.h"

typedef struct {
    ErrorType type;          // Error type
    ErrorSeverity severity;  // Error severity
    uint32_t code;          // Error code
    uint32_t timestamp;     // When error occurred
    SystemState source_state; // State when error occurred
    char message[128];      // Error message
    char context[256];      // Additional context
    bool recoverable;       // Can be recovered from
    uint8_t retry_count;    // Current retry count
    uint32_t backoff_ms;    // Current backoff delay
} ErrorDetails;

// Core error functions
void error_init(void);
void error_report(ErrorType type, ErrorSeverity severity, uint32_t code, const char *message);
void error_report_with_context(ErrorType type, const char* message, const char* context);
ErrorDetails *error_get_last(void);
bool error_is_recoverable(const ErrorDetails *error);

// Validation functions
bool error_code_in_range(ErrorType type, uint32_t code);
const char *error_type_to_string(ErrorType type);
const char *error_severity_to_string(ErrorSeverity severity);

// Add these function declarations
void error_print_last(void);
bool error_requires_reset(const ErrorDetails *error);

#endif // ERROR_H
