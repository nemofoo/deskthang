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
    ERROR_CAT_NONE,
    ERROR_CAT_HARDWARE,    // Hardware-related errors
    ERROR_CAT_PROTOCOL,    // Protocol-related errors
    ERROR_CAT_STATE,       // State machine errors
    ERROR_CAT_COMMAND,     // Command processing errors
    ERROR_CAT_TRANSFER,    // Data transfer errors
    ERROR_CAT_SYSTEM      // System-level errors
} ErrorCategory;

// Error details structure
typedef struct {
    ErrorCategory category;     // Error category
    ErrorSeverity severity;     // Error severity
    uint32_t code;             // Error code
    uint32_t timestamp;        // When error occurred
    SystemState state;         // State when error occurred
    char message[128];         // Error message
    char context[256];         // Additional context
    bool recoverable;          // Can be recovered from
} ErrorDetails;

// Error history entry
typedef struct {
    ErrorDetails error;        // Error details
    uint32_t frequency;        // How often this error occurs
    uint32_t last_seen;        // Last occurrence
    uint32_t first_seen;       // First occurrence
} ErrorHistoryEntry;

// Error statistics
typedef struct {
    uint32_t total_errors;     // Total errors seen
    uint32_t recoverable;      // Recoverable errors
    uint32_t unrecoverable;    // Unrecoverable errors
    uint32_t retries;          // Total retry attempts
    uint32_t recoveries;       // Successful recoveries
} ErrorStats;

// Error management functions
bool error_init(void);
void error_reset(void);

// Error reporting
void error_report(ErrorCategory category, ErrorSeverity severity, uint32_t code, const char *message);
void error_report_with_context(ErrorCategory category, ErrorSeverity severity, uint32_t code, 
                             const char *message, const char *context);

// Error query
ErrorDetails *error_get_last(void);
ErrorDetails *error_get_by_code(uint32_t code);
ErrorHistoryEntry *error_get_history(void);
uint32_t error_get_history_count(void);

// Error statistics
ErrorStats *error_get_stats(void);
void error_update_stats(bool recovered);
void error_reset_stats(void);

// Error classification
bool error_is_recoverable(const ErrorDetails *error);
bool error_requires_reset(const ErrorDetails *error);
ErrorSeverity error_get_severity(uint32_t code);

// Debug support
void error_print_last(void);
void error_print_history(void);
void error_print_stats(void);
const char *error_severity_to_string(ErrorSeverity severity);
const char *error_category_to_string(ErrorCategory category);

#endif // DESKTHANG_ERROR_H
