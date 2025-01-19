#ifndef DESKTHANG_RECOVERY_H
#define DESKTHANG_RECOVERY_H

#include <stdint.h>
#include <stdbool.h>
#include "error.h"

// Recovery strategies
typedef enum {
    RECOVERY_NONE,           // No recovery possible/needed
    RECOVERY_RETRY,          // Simple retry of operation
    RECOVERY_RESET_STATE,    // Reset state machine
    RECOVERY_REINIT,         // Reinitialize subsystem
    RECOVERY_REBOOT         // Full system reboot
} RecoveryStrategy;

// Recovery attempt result
typedef struct {
    bool success;            // Recovery succeeded
    uint32_t duration_ms;    // How long recovery took
    uint32_t attempts;       // Number of attempts made
    char message[128];       // Result message
} RecoveryResult;

// Recovery configuration
typedef struct {
    uint32_t max_retries;          // Maximum retry attempts
    uint32_t base_delay_ms;        // Base delay between retries
    uint32_t max_delay_ms;         // Maximum delay between retries
    bool allow_reboot;             // Whether reboot recovery is allowed
} RecoveryConfig;

// Recovery initialization
bool recovery_init(void);
void recovery_reset(void);

// Recovery configuration
void recovery_configure(const RecoveryConfig *config);
RecoveryConfig *recovery_get_config(void);

// Recovery strategy selection
RecoveryStrategy recovery_get_strategy(const ErrorDetails *error);
bool recovery_is_strategy_allowed(RecoveryStrategy strategy);

// Recovery execution
RecoveryResult recovery_attempt(const ErrorDetails *error);
bool recovery_abort(void);
bool recovery_is_in_progress(void);

// Retry management
bool recovery_should_retry(uint32_t attempt_count);
uint32_t recovery_get_retry_delay(uint32_t attempt_count);
void recovery_wait_before_retry(uint32_t delay_ms);

// Recovery handlers
typedef bool (*RecoveryHandler)(const ErrorDetails *error);

bool recovery_register_handler(RecoveryStrategy strategy, RecoveryHandler handler);
RecoveryHandler recovery_get_handler(RecoveryStrategy strategy);

// Recovery tracking
typedef struct {
    uint32_t total_attempts;     // Total recovery attempts
    uint32_t successful;         // Successful recoveries
    uint32_t failed;            // Failed recoveries
    uint32_t aborted;           // Aborted recoveries
    uint32_t total_retry_time;  // Total time spent in recovery
} RecoveryStats;

RecoveryStats *recovery_get_stats(void);
void recovery_reset_stats(void);

// Debug support
void recovery_print_stats(void);
const char *recovery_strategy_to_string(RecoveryStrategy strategy);
void recovery_log_attempt(const ErrorDetails *error, RecoveryStrategy strategy, bool success);

#endif // DESKTHANG_RECOVERY_H
