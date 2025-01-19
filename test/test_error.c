#include "unity.h"
#include "error/error.h"
#include "error/logging.h"
#include <string.h>

// Mock functions needed by error.c
uint32_t get_system_time(void) {
    return 1000; // Fixed time for testing
}

SystemState state_machine_get_current(void) {
    return STATE_READY;
}

void run_error_tests(void) {
    // Test error initialization
    RUN_TEST(test_error_init);
    RUN_TEST(test_error_reporting);
    RUN_TEST(test_error_recovery);
    RUN_TEST(test_error_severity);
}

void test_error_init(void) {
    // Test initialization
    TEST_ASSERT_TRUE(error_init());
    
    // Verify initial state
    ErrorDetails *error = error_get_last();
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL(ERROR_TYPE_NONE, error->type);
    TEST_ASSERT_EQUAL(0, error->code);
    TEST_ASSERT_EQUAL(0, strlen(error->message));
}

void test_error_reporting(void) {
    // Setup
    error_init();
    
    // Test basic error reporting
    const char *test_message = "Test error message";
    error_report(ERROR_TYPE_PROTOCOL, ERROR_SEVERITY_ERROR, 
                123, test_message);
    
    // Verify error details
    ErrorDetails *error = error_get_last();
    TEST_ASSERT_NOT_NULL(error);
    TEST_ASSERT_EQUAL(ERROR_TYPE_PROTOCOL, error->type);
    TEST_ASSERT_EQUAL(ERROR_SEVERITY_ERROR, error->severity);
    TEST_ASSERT_EQUAL(123, error->code);
    TEST_ASSERT_EQUAL_STRING(test_message, error->message);
    
    // Test error with context
    const char *test_context = "Additional context";
    error_report_with_context(ERROR_TYPE_HARDWARE, 
                            ERROR_SEVERITY_WARNING,
                            456, test_message, test_context);
    
    error = error_get_last();
    TEST_ASSERT_EQUAL(ERROR_TYPE_HARDWARE, error->type);
    TEST_ASSERT_EQUAL_STRING(test_context, error->context);
}

void test_error_recovery(void) {
    // Setup
    error_init();
    
    // Test recoverable error
    error_report(ERROR_TYPE_PROTOCOL, ERROR_SEVERITY_ERROR, 
                789, "Recoverable error");
    
    ErrorDetails *error = error_get_last();
    TEST_ASSERT_TRUE(error_is_recoverable(error));
    TEST_ASSERT_FALSE(error_requires_reset(error));
    
    // Test fatal error
    error_report(ERROR_TYPE_HARDWARE, ERROR_SEVERITY_FATAL, 
                999, "Fatal error");
    
    error = error_get_last();
    TEST_ASSERT_FALSE(error_is_recoverable(error));
    TEST_ASSERT_TRUE(error_requires_reset(error));
}

void test_error_severity(void) {
    // Setup
    error_init();
    
    // Test severity string conversion
    TEST_ASSERT_EQUAL_STRING("INFO", 
        error_severity_to_string(ERROR_SEVERITY_INFO));
    TEST_ASSERT_EQUAL_STRING("WARNING", 
        error_severity_to_string(ERROR_SEVERITY_WARNING));
    TEST_ASSERT_EQUAL_STRING("ERROR", 
        error_severity_to_string(ERROR_SEVERITY_ERROR));
    TEST_ASSERT_EQUAL_STRING("FATAL", 
        error_severity_to_string(ERROR_SEVERITY_FATAL));
} 