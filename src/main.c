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
    // Initialize GPIO for LED
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 1);  // Turn on LED to show we're starting
    
    // Initialize stdio first, before any printing
    stdio_init_all();
    
    // Wait for USB CDC to be ready, blink LED while waiting
    for(int i = 0; i < 6; i++) {  // 3 seconds total
        gpio_put(LED_PIN, i % 2);  // Toggle LED every 500ms
        sleep_ms(500);
    }

    sleep_ms(2000);
    
    // Now start printing
    printf("\n\n\n=== Deskthang Debug Output ===\n");
    fflush(stdout);
    printf("USB CDC should be ready now\n");
    fflush(stdout);
    
    // Initialize core subsystems
    printf("Initializing core subsystems...\n");
    fflush(stdout);
    gpio_put(LED_PIN, 1);  // LED on during initialization
    
    if (!init_subsystems()) {
        // Handle initialization failure
        error_print_last();
        printf("Core initialization failed!\n");
        fflush(stdout);
        while(1) {
            gpio_put(LED_PIN, 1);  // Fast blink to indicate failure
            sleep_ms(100);
            gpio_put(LED_PIN, 0);
            sleep_ms(100);
        }
        return 1;
    }
    
    logging_write("Main", "Core systems initialized successfully");
    printf("Core initialization successful!\n");
    fflush(stdout);
    
    // Now try to initialize display
    printf("Initializing display...\n");
    fflush(stdout);
    if (!display_init(&hw_config, &display_config)) {
        printf("Display initialization failed - continuing without display\n");
        fflush(stdout);
        // Flash LED 3 times to indicate display failure but continue
        for(int i = 0; i < 3; i++) {
            gpio_put(LED_PIN, 0); sleep_ms(100);
            gpio_put(LED_PIN, 1); sleep_ms(100);
        }
    } else {
        printf("Display initialized successfully\n");
        fflush(stdout);
    }
    
    // Try to initialize protocol
    printf("Initializing protocol...\n");
    fflush(stdout);
    if (!protocol_init(&protocol_config)) {
        printf("Protocol initialization failed - continuing without protocol\n");
        fflush(stdout);
        // Flash LED 3 times to indicate protocol failure but continue
        for(int i = 0; i < 3; i++) {
            gpio_put(LED_PIN, 0); sleep_ms(100);
            gpio_put(LED_PIN, 1); sleep_ms(100);
        }
    } else {
        printf("Protocol initialized successfully\n");
        fflush(stdout);
    }
    
    uint32_t heartbeat = 0;
    bool led_state = false;
    
    // Main event loop
    while (1) {
        // Heartbeat every second
        if (heartbeat % 1000 == 0) {
            led_state = !led_state;  // Toggle LED with heartbeat
            gpio_put(LED_PIN, led_state);
            printf("Heartbeat: %lu\n", heartbeat / 1000);
            fflush(stdout);
        }
        heartbeat++;
        sleep_ms(1);
        
        // Process any pending packets
        Packet packet;
        if (packet_receive(&packet)) {
            gpio_put(LED_PIN, 1);  // Flash LED briefly when packet received
            printf("Received packet type: %d\n", packet.header.type);
            fflush(stdout);
            if (!protocol_process_packet(&packet)) {
                // Handle protocol error
                ErrorDetails *error = error_get_last();
                if (error && error_is_recoverable(error)) {
                    RecoveryResult result = recovery_attempt(error);
                    // Log recovery attempt
                    char msg[64];
                    snprintf(msg, sizeof(msg), "%s (Duration: %ums, Attempts: %u)", 
                             result.message, result.duration_ms, result.attempts);
                    logging_write("Recovery", msg);
                    printf("Recovery attempt: %s\n", msg);
                    fflush(stdout);
                }
            }
            sleep_ms(50);  // Keep LED on briefly
            gpio_put(LED_PIN, led_state);  // Return to heartbeat state
        }
        
        // Handle any pending errors
        ErrorDetails *error = error_get_last();
        if (error && error->message[0] != '\0') {  // Only handle non-empty errors
            gpio_put(LED_PIN, 1);  // LED on while handling error
            printf("Error detected: %s\n", error->message);
            fflush(stdout);
            if (error_is_recoverable(error)) {
                RecoveryResult result = recovery_attempt(error);
                // Log recovery attempt
                char msg[64];
                snprintf(msg, sizeof(msg), "%s (Duration: %ums, Attempts: %u)", 
                         result.message, result.duration_ms, result.attempts);
                logging_write("Recovery", msg);
                printf("Recovery attempt: %s\n", msg);
                fflush(stdout);
            }
            gpio_put(LED_PIN, led_state);  // Return to heartbeat state
        }
    }
}
