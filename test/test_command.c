#include "unity.h"
#include "protocol/command.h"

void run_command_tests(void) {
    RUN_TEST(test_command_placeholder);
}

void test_command_placeholder(void) {
    TEST_ASSERT_EQUAL(1, 1);
} 