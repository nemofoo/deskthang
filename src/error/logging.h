#ifndef DESKTHANG_LOGGING_H
#define DESKTHANG_LOGGING_H

#include <stdint.h>
#include <stdbool.h>
#include "error.h"
#include "recovery.h"

// Log levels
typedef enum {
    LOG_LEVEL_NONE,
    LOG_LEVEL_ERROR,    // Errors only
    LOG_LEVEL_WARNING,  // Errors and warnings
    LOG_LEVEL_INFO,     // General information
    LOG_LEVEL_DEBUG,    // Debug information
    LOG_LEVEL_TRACE     // Detailed tracing
} LogLevel;

// Log entry structure
typedef struct {
    uint32_t timestamp;     // Entry timestamp
    LogLevel level;         // Log level
    SystemState state;      // System state
    char module[32];        // Source module
    char message[256];      // Log message
    char context[512];      // Additional context
} LogEntry;

// Log buffer configuration
typedef struct {
    uint32_t max_entries;      // Maximum entries to store
    bool circular;             // Circular buffer behavior
    bool persist;              // Save to flash
    LogLevel min_level;        // Minimum level to log
} LogConfig;

// Log initialization
bool logging_init(void);
void logging_reset(void);

// Configuration
void logging_configure(const LogConfig *config);
LogConfig *logging_get_config(void);
void logging_set_level(LogLevel level);
LogLevel logging_get_level(void);

// Log writing
void logging_write(LogLevel level, const char *module, const char *message);
void logging_write_with_context(LogLevel level, const char *module, 
                              const char *message, const char *context);

// Error logging
void logging_error(const ErrorDetails *error);
void logging_recovery(const RecoveryResult *result);

// Log reading
LogEntry *logging_get_entries(uint32_t *count);
LogEntry *logging_get_last_entry(void);
LogEntry *logging_find_by_level(LogLevel level, uint32_t *count);
LogEntry *logging_find_by_module(const char *module, uint32_t *count);

// Log management
void logging_clear(void);
bool logging_save(void);
bool logging_load(void);

// Log statistics
typedef struct {
    uint32_t total_entries;     // Total entries logged
    uint32_t errors;            // Error entries
    uint32_t warnings;          // Warning entries
    uint32_t buffer_wraps;      // Times buffer wrapped
    uint32_t buffer_usage;      // Current buffer usage %
} LogStats;

LogStats *logging_get_stats(void);
void logging_reset_stats(void);

// Debug support
void logging_print_entry(const LogEntry *entry);
void logging_print_last(void);
void logging_print_all(void);
void logging_print_stats(void);
const char *logging_level_to_string(LogLevel level);

#endif // DESKTHANG_LOGGING_H
