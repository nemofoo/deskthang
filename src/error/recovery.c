#include "recovery.h"
#include "error.h"
#include "logging.h"
#include "../system/time.h"
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"

// Default recovery configuration
static RecoveryConfig g_recovery_config = {
    .max_retries = 8,
    .base_delay_ms = 50,
    .max_delay_ms = 1000,
    .allow_reboot = false
};

// Recovery statistics
static RecoveryStats g_recovery_stats;

// Recovery handlers
static RecoveryHandler g_recovery_handlers[5] = {NULL}; // One for each strategy

// Initialize recovery system
bool recovery_init(void) {
    memset(&g_recovery_stats, 0, sizeof(RecoveryStats));
    memset(g_recovery_handlers, 0, sizeof(g_recovery_handlers));
    return true;
}

// Reset recovery state
void recovery_reset(void) {
    memset(&g_recovery_stats, 0, sizeof(RecoveryStats));
}

// Configure recovery system
void recovery_configure(const RecoveryConfig *config) {
    if (config) {
        memcpy(&g_recovery_config, config, sizeof(RecoveryConfig));
    }
}

RecoveryConfig *recovery_get_config(void) {
    return &g_recovery_config;
}

// Recovery strategy selection
RecoveryStrategy recovery_get_strategy(const ErrorDetails *error) {
    if (!error) {
        return RECOVERY_NONE;
    }
    
    // Fatal errors require reboot if allowed
    if (error->severity == ERROR_SEVERITY_FATAL) {
        return g_recovery_config.allow_reboot ? RECOVERY_REBOOT : RECOVERY_NONE;
    }
    
    // Hardware errors typically need reinitialization
    if (error->type == ERROR_TYPE_HARDWARE) {
        return RECOVERY_REINIT;
    }
    
    // State machine errors need state reset
    if (error->type == ERROR_TYPE_STATE) {
        return RECOVERY_RESET_STATE;
    }
    
    // Most other errors can be retried
    if (error->recoverable) {
        return RECOVERY_RETRY;
    }
    
    return RECOVERY_NONE;
}

bool recovery_is_strategy_allowed(RecoveryStrategy strategy) {
    if (strategy == RECOVERY_REBOOT && !g_recovery_config.allow_reboot) {
        return false;
    }
    return true;
}

// Simplified recovery handlers for prototype
static bool handle_retry_recovery(const ErrorDetails *error) {
    uint32_t delay = recovery_get_retry_delay(g_recovery_stats.total_attempts);
    recovery_wait_before_retry(delay);
    return true;
}

static bool handle_reset_recovery(const ErrorDetails *error) {
    // Reset to known good state
    return true;
}

static bool handle_reinit_recovery(const ErrorDetails *error) {
    // Reinitialize hardware
    return true;
}

// Recovery execution
RecoveryResult recovery_attempt(const ErrorDetails *error) {
    RecoveryResult result = {0};
    result.success = false;
    
    if (!error) {
        strncpy(result.message, "No error details provided", sizeof(result.message) - 1);
        return result;
    }
    
    // Get recovery strategy
    RecoveryStrategy strategy = recovery_get_strategy(error);
    if (strategy == RECOVERY_NONE) {
        strncpy(result.message, "No recovery strategy available", sizeof(result.message) - 1);
        return result;
    }
    
    // Track attempt
    g_recovery_stats.total_attempts++;
    uint32_t start_time = deskthang_time_get_ms();
    
    // Execute recovery based on strategy
    switch (strategy) {
        case RECOVERY_RETRY:
            result.success = handle_retry_recovery(error);
            strncpy(result.message, "Retry recovery", sizeof(result.message) - 1);
            break;
            
        case RECOVERY_RESET_STATE:
            result.success = handle_reset_recovery(error);
            strncpy(result.message, "Reset state recovery", sizeof(result.message) - 1);
            break;
            
        case RECOVERY_REINIT:
            result.success = handle_reinit_recovery(error);
            strncpy(result.message, "Reinit recovery", sizeof(result.message) - 1);
            break;
            
        case RECOVERY_REBOOT:
            if (g_recovery_config.allow_reboot) {
                strncpy(result.message, "Rebooting system", sizeof(result.message) - 1);
                result.success = true;
                // System will reboot, this won't return
                reset_usb_boot(0, 0);
            }
            break;
            
        default:
            strncpy(result.message, "Unknown recovery strategy", sizeof(result.message) - 1);
            break;
    }
    
    result.duration_ms = deskthang_time_get_ms() - start_time;
    result.attempts = 1;
    
    // Update stats
    if (result.success) {
        g_recovery_stats.successful++;
    } else {
        g_recovery_stats.failed++;
    }
    
    // Log result
    recovery_log_attempt(error, strategy, result.success);
    
    return result;
}

bool recovery_abort(void) {
    g_recovery_stats.aborted++;
    return true;
}

bool recovery_is_in_progress(void) {
    // TODO: Implement in-progress tracking
    return false;
}

// Retry management
bool recovery_should_retry(uint32_t attempt_count) {
    return attempt_count < g_recovery_config.max_retries;
}

uint32_t recovery_get_retry_delay(uint32_t attempt_count) {
    uint32_t delay = g_recovery_config.base_delay_ms;
    
    // Exponential backoff
    for (uint32_t i = 0; i < attempt_count && delay < g_recovery_config.max_delay_ms; i++) {
        delay *= 2;
    }
    
    return (delay > g_recovery_config.max_delay_ms) ? 
        g_recovery_config.max_delay_ms : delay;
}

void recovery_wait_before_retry(uint32_t delay_ms) {
    sleep_ms(delay_ms);
}

// Recovery handlers
bool recovery_register_handler(RecoveryStrategy strategy, RecoveryHandler handler) {
    if (strategy >= RECOVERY_NONE && strategy <= RECOVERY_REBOOT) {
        g_recovery_handlers[strategy] = handler;
        return true;
    }
    return false;
}

RecoveryHandler recovery_get_handler(RecoveryStrategy strategy) {
    if (strategy >= RECOVERY_NONE && strategy <= RECOVERY_REBOOT) {
        return g_recovery_handlers[strategy];
    }
    return NULL;
}

// Recovery statistics
RecoveryStats *recovery_get_stats(void) {
    return &g_recovery_stats;
}

void recovery_reset_stats(void) {
    memset(&g_recovery_stats, 0, sizeof(RecoveryStats));
}

// Debug support
void recovery_print_stats(void) {
    printf("Recovery Statistics:\n");
    printf("  Total Attempts: %u\n", g_recovery_stats.total_attempts);
    printf("  Successful: %u\n", g_recovery_stats.successful);
    printf("  Failed: %u\n", g_recovery_stats.failed);
    printf("  Aborted: %u\n", g_recovery_stats.aborted);
    printf("  Total Retry Time: %ums\n", g_recovery_stats.total_retry_time);
}

const char *recovery_strategy_to_string(RecoveryStrategy strategy) {
    switch (strategy) {
        case RECOVERY_NONE:       return "NONE";
        case RECOVERY_RETRY:      return "RETRY";
        case RECOVERY_RESET_STATE:return "RESET_STATE";
        case RECOVERY_REINIT:     return "REINIT";
        case RECOVERY_REBOOT:     return "REBOOT";
        default:                  return "UNKNOWN";
    }
}

void recovery_log_attempt(const ErrorDetails *error, RecoveryStrategy strategy, bool success) {
    char message[256];
    snprintf(message, sizeof(message), "Recovery attempt: %s, Strategy: %s, Result: %s",
             error->message,
             recovery_strategy_to_string(strategy),
             success ? "Success" : "Failed");
             
    logging_write("Recovery", message);
    
    // Should add:
    // - Timing information
    // - Retry count
    // - Backoff delay used
    // - More detailed error context
}
