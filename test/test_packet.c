#include "unity.h"
#include "protocol/packet.h"

void run_packet_tests(void) {
    RUN_TEST(test_packet_placeholder);
}

void test_packet_placeholder(void) {
    TEST_ASSERT_EQUAL(1, 1);
} 