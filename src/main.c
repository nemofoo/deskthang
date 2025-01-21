#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/hardware.h"
#include "hardware/display.h"
#include "hardware/serial.h"
#include "protocol/protocol.h"
#include "error/error.h"
#include "error/logging.h"
#include "error/recovery.h"
#include "protocol/packet.h"

// Hardware configuration
const HardwareConfig hw_config = {
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
const DisplayConfig display_config = {
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
    const uint LED_PIN = 25;  // Make LED_PIN accessible here
    
    // Initialize error handling first (doesn't depend on anything)
    error_init();
    gpio_put(LED_PIN, 0); sleep_ms(200); gpio_put(LED_PIN, 1);  // 1 blink for error init
    sleep_ms(1000);  // Long pause after section
    
    // Initialize serial directly (don't call stdio_init_all() since it's done in main)
    if (!serial_init()) {
        return false;
    }
    gpio_put(LED_PIN, 0); sleep_ms(200); gpio_put(LED_PIN, 1);  // 1 more blink for serial
    sleep_ms(1000);
    
    // Now initialize logging (depends on serial)
    if (!logging_init()) {
        return false;
    }
    gpio_put(LED_PIN, 0); sleep_ms(200); 
    gpio_put(LED_PIN, 1); sleep_ms(200); 
    gpio_put(LED_PIN, 0); sleep_ms(200);
    gpio_put(LED_PIN, 1);  // 2 blinks for logging init
    sleep_ms(1000);  // Long pause after section
    
    // From here on we can use logging for debug output
    logging_write("Init", "Logging system initialized");
    
    // Initialize recovery system
    if (!recovery_init()) {
        logging_write("Init", "Recovery system initialization failed");
        return false;
    }
    gpio_put(LED_PIN, 0); sleep_ms(200);
    gpio_put(LED_PIN, 1); sleep_ms(200);
    gpio_put(LED_PIN, 0); sleep_ms(200);
    gpio_put(LED_PIN, 1); sleep_ms(200);
    gpio_put(LED_PIN, 0); sleep_ms(200);
    gpio_put(LED_PIN, 1);  // 3 blinks for recovery init
    sleep_ms(1000);  // Long pause after section
    
    logging_write("Init", "Recovery system initialized");
    
    // Configure recovery using protocol-defined constants
    recovery_configure(&recovery_config);
    
    // Register recovery handlers
    recovery_register_handler(RECOVERY_RETRY, retry_handler);
    recovery_register_handler(RECOVERY_RESET_STATE, reset_handler);
    recovery_register_handler(RECOVERY_REINIT, reinit_handler);
    
    // Initialize basic hardware (GPIO, etc)
    if (!hardware_init(&hw_config)) {
        logging_write("Init", "Hardware initialization failed");
        error_report(ERROR_TYPE_HARDWARE, ERROR_SEVERITY_FATAL, 1, "Hardware initialization failed");
        return false;
    }
    for(int i = 0; i < 4; i++) {  // 4 blinks for hardware init
        gpio_put(LED_PIN, 0); sleep_ms(200);
        gpio_put(LED_PIN, 1); sleep_ms(200);
    }
    sleep_ms(1000);  // Long pause after section
    
    logging_write("Init", "Core subsystems initialized successfully");
    return true;
}

int main() {
    // Initialize stdio for initial printf only
    stdio_init_all();
    printf("DeskThang starting up...\n");
    fflush(stdout);

    // Initialize GPIO for LED
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);

    // Initialize error handling first (doesn't depend on anything)
    error_init();
    gpio_put(LED_PIN, 0); sleep_ms(200); gpio_put(LED_PIN, 1);  // 1 blink for error init
    sleep_ms(1000);  // Long pause after section
    
    // Initialize serial directly
    if (!serial_init()) {
        return false;
    }
    gpio_put(LED_PIN, 0); sleep_ms(200); gpio_put(LED_PIN, 1);  // 1 more blink for serial
    sleep_ms(1000);
    
    // Initialize logging (depends on serial)
    if (!logging_init()) {
        return false;
    }
    gpio_put(LED_PIN, 0); sleep_ms(200); 
    gpio_put(LED_PIN, 1); sleep_ms(200); 
    gpio_put(LED_PIN, 0); sleep_ms(200);
    gpio_put(LED_PIN, 1);  // 2 blinks for logging init
    sleep_ms(1000);  // Long pause after section

    // Initialize packet system
    if (!packet_buffer_init()) {
        return false;
    }

    // Switch to debug packets for all future logging
    logging_enable_debug_packets();
    
    // From here on we use debug packets for all output
    logging_write("Init", "System initialized, switching to debug packets");
    
    // Rest of initialization...
    if (!init_subsystems()) {
        return false;
    }

    // Initialize core subsystems
    printf("Initializing state machine...\n");
    fflush(stdout);
    gpio_put(LED_PIN, 1);  // LED on during initialization

    if (!state_machine_init()) {
        // Handle initialization failure
        printf("State machine initialization failed!\n");
        fflush(stdout);
        while(1) {
            gpio_put(LED_PIN, 1);  // Fast blink to indicate failure
            sleep_ms(100);
            gpio_put(LED_PIN, 0);
            sleep_ms(100);
        }
        return 1;
    }

    logging_write("Main", "State machine initialized successfully");
    printf("State machine initialization successful!\n");
    fflush(stdout);

    // Let the state machine handle initialization
    printf("Starting state machine initialization sequence...\n");
    fflush(stdout);

    // Main event loop
    uint32_t heartbeat = 0;
    bool led_state = false;
    Packet packet;  // Moved outside switch

    while (1) {
        SystemState current_state = state_machine_get_current();
        
        // Process state-specific actions
        switch (current_state) {
            case STATE_HARDWARE_INIT:
                // Hardware init is handled by entry action
                break;
                
            case STATE_DISPLAY_INIT:
                // If display is initialized, transition to IDLE
                if (display_is_initialized()) {
                    state_machine_transition(STATE_IDLE, CONDITION_DISPLAY_READY);
                }
                break;
                
            case STATE_IDLE:
            case STATE_READY:
                // Process packets in these states
                if (packet_receive(&packet)) {
                    gpio_put(LED_PIN, 1);  // Flash LED briefly when packet received
                    if (!protocol_process_packet(&packet)) {
                        // Handle protocol error through state machine
                        state_machine_transition(STATE_ERROR, CONDITION_ERROR);
                    }
                    sleep_ms(50);  // Keep LED on briefly
                    gpio_put(LED_PIN, led_state);  // Return to heartbeat state
                } else {
                    sleep_ms(10);  // Add small delay when no packet received to prevent tight-looping
                }
                break;
                
            case STATE_ERROR:
                // Handle error recovery
                if (state_machine_handle_error()) {
                    // Error handled, state machine will transition
                    continue;
                }
                break;
        }
        
        // Heartbeat every second
        if (heartbeat % 1000 == 0) {
            led_state = !led_state;  // Toggle LED with heartbeat
            gpio_put(LED_PIN, led_state);
            
            // Send heartbeat through debug packet instead of printf
            char message[64];
            snprintf(message, sizeof(message), "Heartbeat: %lu, State: %s", heartbeat / 1000, state_to_string(current_state));
            Packet debug_packet;
            if (packet_create_debug(&debug_packet, "SYSTEM", message)) {
                packet_transmit(&debug_packet);
            }
        }
        heartbeat++;
        sleep_ms(1);
    }
}
