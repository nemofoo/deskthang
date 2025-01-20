#ifndef ERROR_H
#define ERROR_H

#include <stdint.h>
#include <stdbool.h>
#include "../common/deskthang_constants.h"

// Error types as defined in protocol_architecture.md
// Error code ranges are defined in deskthang_constants.h

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
    ErrorType type;          
    ErrorSeverity severity;  
    uint32_t code;          
    uint32_t timestamp;     
    SystemState source_state;
    char message[ERROR_MESSAGE_SIZE];    // Use new constant
    char context[ERROR_CONTEXT_SIZE];    // Use new constant
    bool recoverable;       
    uint8_t retry_count;    
    uint32_t backoff_ms;    
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
