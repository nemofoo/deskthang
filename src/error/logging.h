#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>
#include "error.h"

// Configuration structure
typedef struct {
    bool enabled;
} LogConfig;

// Core logging functions
bool logging_init(void);
void logging_write(const char *module, const char *message);
void logging_write_with_context(const char *module, const char *message, const char *context);

// Error logging functions
void logging_error(const ErrorDetails *error);
void logging_error_with_context(const char *module, uint32_t code, const char *message, const char *context);

// Configuration functions
void logging_set_enabled(bool enabled);
bool logging_is_enabled(void);

#endif // LOGGING_H
