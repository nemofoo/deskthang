#include "unity.h"
#include <stdio.h>

// Function declarations for test groups
void run_error_tests(void);
void run_logging_tests(void);
void run_recovery_tests(void);
void run_packet_tests(void);
void run_command_tests(void);
void run_protocol_tests(void);
void run_state_tests(void);

// Required by Unity
void setUp(void) {
    // Common setup code before each test
}

void tearDown(void) {
    // Common cleanup code after each test
}

int main(void) {
    UNITY_BEGIN();

    // Run all test groups
    run_error_tests();
    run_logging_tests();
    run_recovery_tests();
    run_packet_tests();
    run_command_tests();
    run_protocol_tests();
    run_state_tests();

    return UNITY_END();
} 