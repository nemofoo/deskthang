#include "unity.h"
#include "state/state.h"

// Forward declarations
void test_state_placeholder(void);

void run_state_tests(void) {
    RUN_TEST(test_state_placeholder);
}

void test_state_placeholder(void) {
    TEST_ASSERT_EQUAL(1, 1);
} 