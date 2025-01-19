#include "unity.h"
#include "error/logging.h"

void run_logging_tests(void) {
    RUN_TEST(test_logging_placeholder);
}

void test_logging_placeholder(void) {
    TEST_ASSERT_EQUAL(1, 1);
} 