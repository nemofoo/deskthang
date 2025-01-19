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
    .spi_port = 0,
    .spi_baud = 10000000,  // 10MHz
    .pins = {
        .mosi = 19,
        .sck = 18,
        .cs = 17,
        .dc = 16,
        .rst = 20
    },
    .timing = {
        .reset_pulse_us = 10000,   // 10ms reset pulse
        .init_delay_ms = 120,      // 120ms init delay
        .cmd_delay_us = 10         // 10us command delay
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
    if (!error_init()) {
        return false;
    }
    
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
    if (!protocol_init()) {
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
                    logging_recovery(&result);
                }
            }
        }
        
        // Handle any pending errors
        ErrorDetails *error = error_get_last();
        if (error) {
            if (error_is_recoverable(error)) {
                RecoveryResult result = recovery_attempt(error);
                logging_recovery(&result);
            } else if (error_requires_reset(error)) {
                logging_write("Main", "Fatal error, resetting system");
                hardware_reset();
            }
        }
        
        // Yield to allow USB processing
        sleep_ms(1); // Minimum polling interval
    }
}
