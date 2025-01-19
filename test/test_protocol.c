#include "unity.h"
#include "protocol/protocol.h"

void run_protocol_tests(void) {
    RUN_TEST(test_protocol_placeholder);
}

void test_protocol_placeholder(void) {
    TEST_ASSERT_EQUAL(1, 1);
} 