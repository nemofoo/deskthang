#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/hardware.h"
#include "hardware/display.h"
#include "protocol/protocol.h"
#include "error/error.h"
#include "error/logging.h"
#include "error/recovery.h"
#include "protocol/packet.h"

// Hardware configuration
static const HardwareConfig hw_config = {
    .spi_port = DISPLAY_SPI_PORT,
    .spi_baud = DISPLAY_SPI_BAUD,
    .pins = {
        .mosi = DISPLAY_PIN_MOSI,
        .sck = DISPLAY_PIN_SCK,
        .cs = DISPLAY_PIN_CS,
        .dc = DISPLAY_PIN_DC,
        .rst = DISPLAY_PIN_RST
    },
    .timing = {
        .reset_pulse_us = DISPLAY_RESET_PULSE_US,
        .init_delay_ms = DISPLAY_INIT_DELAY_MS,
        .cmd_delay_us = DISPLAY_CMD_DELAY_US
    }
};

// Display configuration
static const DisplayConfig display_config = {
    .orientation = DISPLAY_ORIENTATION_0,
    .brightness = 255,
    .inverted = false
};

// Recovery configuration
static const RecoveryConfig recovery_config = {
    .max_retries = MAX_RETRIES,
    .base_delay_ms = MIN_RETRY_DELAY_MS,
    .max_delay_ms = MAX_RETRY_DELAY_MS,
    .allow_reboot = false
};

// Add protocol configuration
const ProtocolConfig protocol_config = {
    .version = 1,
    // ... other protocol configuration ...
};

// Recovery handlers
static bool retry_handler(const ErrorDetails *error) {
    // Implement retry logic
    return true;
}

static bool reset_handler(const ErrorDetails *error) {
    // Implement state reset logic
    return true;
}

static bool reinit_handler(const ErrorDetails *error) {
    // Implement reinitialization logic
    return hardware_reset();
}

// Initialize subsystems
static bool init_subsystems(void) {
    // Initialize error handling first
    error_init();
    
    // Initialize logging
    if (!logging_init()) {
        return false;
    }
    
    // Initialize recovery system
    if (!recovery_init()) {
        return false;
    }
    
    // Configure recovery using protocol-defined constants
    recovery_configure(&recovery_config);
    
    // Register recovery handlers
    recovery_register_handler(RECOVERY_RETRY, retry_handler);
    recovery_register_handler(RECOVERY_RESET_STATE, reset_handler);
    recovery_register_handler(RECOVERY_REINIT, reinit_handler);
    
    // Initialize hardware
    if (!hardware_init(&hw_config)) {
        error_report(ERROR_TYPE_HARDWARE, ERROR_SEVERITY_FATAL, 1, "Hardware initialization failed");
        return false;
    }
    
    // Initialize display
    if (!display_init(&hw_config, &display_config)) {
        error_report(ERROR_TYPE_HARDWARE, ERROR_SEVERITY_FATAL, 2, "Display initialization failed");
        return false;
    }
    
    // Initialize protocol handler
    if (!protocol_init(&protocol_config)) {
        error_report(ERROR_TYPE_PROTOCOL, ERROR_SEVERITY_FATAL, 3, "Protocol initialization failed");
        return false;
    }
    
    return true;
}

int main() {
    // Initialize stdio
    stdio_init_all();
    sleep_ms(2 * BASE_TIMEOUT_MS); // Wait for USB CDC with protocol-defined timeout
    
    // Initialize all subsystems
    if (!init_subsystems()) {
        // Handle initialization failure
        error_print_last();
        return 1;
    }
    
    logging_write("Main", "System initialized successfully");
    
    // Main event loop
    while (1) {
        // Process any pending packets
        Packet packet;
        if (packet_receive(&packet)) {
            if (!protocol_process_packet(&packet)) {
                // Handle protocol error
                ErrorDetails *error = error_get_last();
                if (error && error_is_recoverable(error)) {
                    RecoveryResult result = recovery_attempt(error);
                    // Log recovery attempt
                    char context[MESSAGE_CONTEXT_SIZE];
                    snprintf(context, sizeof(context), "Duration: %ums, Attempts: %u",
                            result.duration_ms, result.attempts);
                    logging_write_with_context("Recovery", result.message, context);
                }
            }
        }
        
        // Handle any pending errors
        ErrorDetails *error = error_get_last();
        if (error) {
            if (error_is_recoverable(error)) {
                RecoveryResult result = recovery_attempt(error);
                // Log recovery attempt
                char context[MESSAGE_CONTEXT_SIZE];
                snprintf(context, sizeof(context), "Duration: %ums, Attempts: %u",
                        result.duration_ms, result.attempts);
                logging_write_with_context("Recovery", result.message, context);
            } else if (error_requires_reset(error)) {
                logging_write("Main", "Fatal error, resetting system");
                hardware_reset();
            }
        }
        
        // Yield to allow USB processing
        sleep_ms(1); // Minimum polling interval
    }
}
