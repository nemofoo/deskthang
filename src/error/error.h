#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>
#include <stdbool.h>
#include "../protocol/protocol_constants.h"

// Error types as defined in protocol_architecture.md
// Error code ranges as defined in protocol_architecture.md
#define ERROR_CODE_HARDWARE_START 1000
#define ERROR_CODE_HARDWARE_END   1999
#define ERROR_CODE_PROTOCOL_START 2000
#define ERROR_CODE_PROTOCOL_END   2999
#define ERROR_CODE_STATE_START    3000
#define ERROR_CODE_STATE_END      3999
#define ERROR_CODE_COMMAND_START  4000
#define ERROR_CODE_COMMAND_END    4999
#define ERROR_CODE_TRANSFER_START 5000
#define ERROR_CODE_TRANSFER_END   5999
#define ERROR_CODE_SYSTEM_START   6000
#define ERROR_CODE_SYSTEM_END     6999

typedef enum {
    ERROR_TYPE_NONE = 0,
    ERROR_TYPE_HARDWARE = ERROR_CODE_HARDWARE_START,
    ERROR_TYPE_PROTOCOL = ERROR_CODE_PROTOCOL_START,
    ERROR_TYPE_STATE = ERROR_CODE_STATE_START,
    ERROR_TYPE_COMMAND = ERROR_CODE_COMMAND_START,
    ERROR_TYPE_TRANSFER = ERROR_CODE_TRANSFER_START,
    ERROR_TYPE_SYSTEM = ERROR_CODE_SYSTEM_START
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
    char message[MAX_PACKET_SIZE/4];    // Error message (128 bytes)
    char context[MAX_PACKET_SIZE/2];    // Additional context (256 bytes)
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
