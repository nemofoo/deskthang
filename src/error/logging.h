#ifndef LOGGING_H
#define LOGGING_H

#include <stdbool.h>
#include "error.h"

// Initialize logging system
bool logging_init(void);

// Enable debug packet mode for logging
void logging_enable_debug_packets(void);

// Basic logging functions
void logging_write(const char *module, const char *message);
void logging_write_with_context(const char *module, const char *message, const char *context);

// Error logging with details
void logging_error_details(const ErrorDetails *error);

#endif // LOGGING_H
