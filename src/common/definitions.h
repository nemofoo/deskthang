#ifndef DESKTHANG_DEFINITIONS_H
#define DESKTHANG_DEFINITIONS_H

// Recovery strategies
typedef enum {
    RECOVERY_NONE,           // No recovery possible/needed
    RECOVERY_RETRY,          // Simple retry of operation
    RECOVERY_RESET_STATE,    // Reset state machine
    RECOVERY_REINIT,         // Reinitialize subsystem
    RECOVERY_REBOOT          // Full system reboot
} RecoveryStrategy;

#endif // DESKTHANG_DEFINITIONS_H
