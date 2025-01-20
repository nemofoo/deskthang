#include "unity.h"

void setUp(void) {
    // Setup function called before each test
}

void tearDown(void) {
    // Teardown function called after each test
}

void test_sanity(void) {
    TEST_ASSERT_EQUAL(1, 1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_sanity);
    return UNITY_END();
} 