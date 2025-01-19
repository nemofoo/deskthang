#include "logging.h"
#include <string.h>
#include <stdio.h>

// Default log configuration
static LogConfig g_log_config = {
    .max_entries = 128,
    .circular = true,
    .persist = false,
    .min_level = LOG_LEVEL_ERROR
};

// Log buffer
static LogEntry *g_log_buffer = NULL;
static uint32_t g_log_count = 0;
static uint32_t g_log_index = 0;

// Log statistics
static LogStats g_log_stats;

// Initialize logging system
bool logging_init(void) {
    // Allocate log buffer
    g_log_buffer = (LogEntry *)malloc(g_log_config.max_entries * sizeof(LogEntry));
    if (!g_log_buffer) {
        return false;
    }
    
    memset(g_log_buffer, 0, g_log_config.max_entries * sizeof(LogEntry));
    memset(&g_log_stats, 0, sizeof(LogStats));
    
    g_log_count = 0;
    g_log_index = 0;
    
    return true;
}

// Reset logging system
void logging_reset(void) {
    if (g_log_buffer) {
        memset(g_log_buffer, 0, g_log_config.max_entries * sizeof(LogEntry));
    }
    memset(&g_log_stats, 0, sizeof(LogStats));
    g_log_count = 0;
    g_log_index = 0;
}

// Configure logging
void logging_configure(const LogConfig *config) {
    if (config) {
        // Save old config
        bool size_changed = (config->max_entries != g_log_config.max_entries);
        
        // Update config
        memcpy(&g_log_config, config, sizeof(LogConfig));
        
        // Reallocate buffer if size changed
        if (size_changed && g_log_buffer) {
            LogEntry *new_buffer = (LogEntry *)realloc(g_log_buffer, 
                                                     g_log_config.max_entries * sizeof(LogEntry));
            if (new_buffer) {
                g_log_buffer = new_buffer;
                if (g_log_count > g_log_config.max_entries) {
                    g_log_count = g_log_config.max_entries;
                }
            }
        }
    }
}

LogConfig *logging_get_config(void) {
    return &g_log_config;
}

void logging_set_level(LogLevel level) {
    g_log_config.min_level = level;
}

LogLevel logging_get_level(void) {
    return g_log_config.min_level;
}

// Log writing
void logging_write(LogLevel level, const char *module, const char *message) {
    logging_write_with_context(level, module, message, NULL);
}

void logging_write_with_context(LogLevel level, const char *module, 
                              const char *message, const char *context) {
    // Check log level
    if (level < g_log_config.min_level) {
        return;
    }
    
    // Get next log entry
    uint32_t index = g_log_index;
    if (g_log_count >= g_log_config.max_entries) {
        if (g_log_config.circular) {
            // Wrap around in circular mode
            g_log_stats.buffer_wraps++;
        } else {
            // Drop entry in non-circular mode
            return;
        }
    }
    
    // Fill entry
    LogEntry *entry = &g_log_buffer[index];
    entry->timestamp = get_system_time();
    entry->level = level;
    entry->state = state_machine_get_current();
    
    if (module) {
        strncpy(entry->module, module, sizeof(entry->module) - 1);
    } else {
        entry->module[0] = '\0';
    }
    
    if (message) {
        strncpy(entry->message, message, sizeof(entry->message) - 1);
    } else {
        entry->message[0] = '\0';
    }
    
    if (context) {
        strncpy(entry->context, context, sizeof(entry->context) - 1);
    } else {
        entry->context[0] = '\0';
    }
    
    // Update indices
    g_log_index = (g_log_index + 1) % g_log_config.max_entries;
    if (g_log_count < g_log_config.max_entries) {
        g_log_count++;
    }
    
    // Update statistics
    g_log_stats.total_entries++;
    if (level == LOG_LEVEL_ERROR) {
        g_log_stats.errors++;
    } else if (level == LOG_LEVEL_WARNING) {
        g_log_stats.warnings++;
    }
    
    // Calculate buffer usage
    g_log_stats.buffer_usage = (g_log_count * 100) / g_log_config.max_entries;
    
    // Save to flash if configured
    if (g_log_config.persist) {
        logging_save();
    }
}

// Error logging
void logging_error(const ErrorDetails *error) {
    if (!error) {
        return;
    }
    
    char context[512];
    snprintf(context, sizeof(context),
             "Category: %s, Severity: %s, Code: %u, Recoverable: %s",
             error_category_to_string(error->category),
             error_severity_to_string(error->severity),
             error->code,
             error->recoverable ? "Yes" : "No");
             
    logging_write_with_context(LOG_LEVEL_ERROR,
                              "Error",
                              error->message,
                              context);
}

void logging_recovery(const RecoveryResult *result) {
    if (!result) {
        return;
    }
    
    char context[512];
    snprintf(context, sizeof(context),
             "Duration: %ums, Attempts: %u",
             result->duration_ms,
             result->attempts);
             
    logging_write_with_context(result->success ? LOG_LEVEL_INFO : LOG_LEVEL_ERROR,
                              "Recovery",
                              result->message,
                              context);
}

// Log reading
LogEntry *logging_get_entries(uint32_t *count) {
    if (count) {
        *count = g_log_count;
    }
    return g_log_buffer;
}

LogEntry *logging_get_last_entry(void) {
    if (g_log_count == 0) {
        return NULL;
    }
    uint32_t last_index = (g_log_index + g_log_config.max_entries - 1) % g_log_config.max_entries;
    return &g_log_buffer[last_index];
}

LogEntry *logging_find_by_level(LogLevel level, uint32_t *count) {
    if (!count) {
        return NULL;
    }
    
    *count = 0;
    for (uint32_t i = 0; i < g_log_count; i++) {
        if (g_log_buffer[i].level == level) {
            (*count)++;
        }
    }
    
    if (*count == 0) {
        return NULL;
    }
    
    LogEntry *results = (LogEntry *)malloc(*count * sizeof(LogEntry));
    if (!results) {
        *count = 0;
        return NULL;
    }
    
    uint32_t result_index = 0;
    for (uint32_t i = 0; i < g_log_count; i++) {
        if (g_log_buffer[i].level == level) {
            memcpy(&results[result_index++], &g_log_buffer[i], sizeof(LogEntry));
        }
    }
    
    return results;
}

LogEntry *logging_find_by_module(const char *module, uint32_t *count) {
    if (!module || !count) {
        return NULL;
    }
    
    *count = 0;
    for (uint32_t i = 0; i < g_log_count; i++) {
        if (strcmp(g_log_buffer[i].module, module) == 0) {
            (*count)++;
        }
    }
    
    if (*count == 0) {
        return NULL;
    }
    
    LogEntry *results = (LogEntry *)malloc(*count * sizeof(LogEntry));
    if (!results) {
        *count = 0;
        return NULL;
    }
    
    uint32_t result_index = 0;
    for (uint32_t i = 0; i < g_log_count; i++) {
        if (strcmp(g_log_buffer[i].module, module) == 0) {
            memcpy(&results[result_index++], &g_log_buffer[i], sizeof(LogEntry));
        }
    }
    
    return results;
}

// Log management
void logging_clear(void) {
    if (g_log_buffer) {
        memset(g_log_buffer, 0, g_log_config.max_entries * sizeof(LogEntry));
    }
    g_log_count = 0;
    g_log_index = 0;
}

bool logging_save(void) {
    // TODO: Implement flash storage
    return true;
}

bool logging_load(void) {
    // TODO: Implement flash loading
    return true;
}

// Log statistics
LogStats *logging_get_stats(void) {
    return &g_log_stats;
}

void logging_reset_stats(void) {
    memset(&g_log_stats, 0, sizeof(LogStats));
}

// Debug support
void logging_print_entry(const LogEntry *entry) {
    if (!entry) {
        return;
    }
    
    printf("[%u] %s: %s (%s)\n",
           entry->timestamp,
           logging_level_to_string(entry->level),
           entry->message,
           entry->module);
           
    if (entry->context[0]) {
        printf("  Context: %s\n", entry->context);
    }
}

void logging_print_last(void) {
    LogEntry *entry = logging_get_last_entry();
    if (entry) {
        logging_print_entry(entry);
    }
}

void logging_print_all(void) {
    printf("Log Entries (%u):\n", g_log_count);
    for (uint32_t i = 0; i < g_log_count; i++) {
        logging_print_entry(&g_log_buffer[i]);
    }
}

void logging_print_stats(void) {
    printf("Log Statistics:\n");
    printf("  Total Entries: %u\n", g_log_stats.total_entries);
    printf("  Errors: %u\n", g_log_stats.errors);
    printf("  Warnings: %u\n", g_log_stats.warnings);
    printf("  Buffer Wraps: %u\n", g_log_stats.buffer_wraps);
    printf("  Buffer Usage: %u%%\n", g_log_stats.buffer_usage);
}

const char *logging_level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_NONE:    return "NONE";
        case LOG_LEVEL_ERROR:   return "ERROR";
        case LOG_LEVEL_WARNING: return "WARNING";
        case LOG_LEVEL_INFO:    return "INFO";
        case LOG_LEVEL_DEBUG:   return "DEBUG";
        case LOG_LEVEL_TRACE:   return "TRACE";
        default:                return "UNKNOWN";
    }
}
