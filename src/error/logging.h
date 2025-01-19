#ifndef DESKTHANG_LOGGING_H
#define DESKTHANG_LOGGING_H

#include <stdint.h>
#include <stdbool.h>
#include "error.h"
#include "recovery.h"

// Configuration
typedef struct {
    bool enabled;         // Enable debug output
} LogConfig;

// Core functions
bool logging_init(void);

// Logging functions
void logging_write(const char *module, const char *message);
void logging_write_with_context(const char *module, const char *message, const char *context);

// Error logging
void logging_error(const ErrorDetails *error);
void logging_recovery(const RecoveryResult *result);

#endif // DESKTHANG_LOGGING_H
