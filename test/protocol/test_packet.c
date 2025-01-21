#include "unity.h"
#include "../mocks/mock_packet.h"

void setUp(void) {
    // Initialize packet buffer before each test
    packet_buffer_init();
}

void tearDown(void) {
    // Reset packet buffer after each test
    packet_buffer_reset();
}

void test_packet_validate_type(void) {
    Packet packet;
    
    // Test valid packet types
    packet.header.packet_type = PACKET_TYPE_SYNC;
    TEST_ASSERT_TRUE(packet_validate(&packet));
    
    packet.header.packet_type = PACKET_TYPE_SYNC_ACK;
    TEST_ASSERT_TRUE(packet_validate(&packet));
    
    packet.header.packet_type = PACKET_TYPE_COMMAND;
    TEST_ASSERT_TRUE(packet_validate(&packet));
    
    packet.header.packet_type = PACKET_TYPE_DATA;
    TEST_ASSERT_TRUE(packet_validate(&packet));
    
    // Test invalid packet type
    packet.header.packet_type = 0xFF;  // Invalid type
    TEST_ASSERT_FALSE(packet_validate(&packet));
    
    // Test null packet
    TEST_ASSERT_FALSE(packet_validate(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_packet_validate_type);
    return UNITY_END();
} 