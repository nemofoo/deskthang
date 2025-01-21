#include <unity.h>
#include <string.h>
#include "../../src/protocol/transfer.h"
#include "../../src/protocol/packet.h"
#include "../../src/common/deskthang_constants.h"
#include "../mocks/mock_time.h"

// Test fixtures
static TransferContext *g_context;
// "123456789" is a standard test vector for CRC32
static uint8_t test_data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
static const uint32_t test_checksum = 0xCBF43926; // Known CRC32 of "123456789"

void setUp(void) {
    mock_time_set(1000);  // Start at t=1000ms
    transfer_init();
    g_context = transfer_get_context();
}

void tearDown(void) {
    transfer_reset();
}

// Test checksum validation
void test_transfer_validate_checksum_valid(void) {
    uint32_t crc = 0xFFFFFFFF;
    printf("Initial CRC: 0x%08X\n", crc);
    
    for (size_t i = 0; i < sizeof(test_data); i++) {
        uint8_t byte = test_data[i];
        uint8_t index = (crc ^ byte) & 0xFF;
        uint32_t old_crc = crc;
        crc = (crc >> 8) ^ crc32_table[index];
        printf("Byte 0x%02X ('%c'): index=0x%02X, crc=0x%08X -> 0x%08X\n", 
               byte, byte, index, old_crc, crc);
    }
    
    crc = crc ^ 0xFFFFFFFF;
    printf("Final CRC: 0x%08X (expected 0x%08X)\n", crc, test_checksum);
    
    bool result = transfer_validate_checksum(test_data, sizeof(test_data), test_checksum);
    printf("Function result: %s\n", result ? "true" : "false");
    
    TEST_ASSERT_TRUE(result);
}

void test_transfer_validate_checksum_invalid(void) {
    TEST_ASSERT_FALSE(transfer_validate_checksum(test_data, sizeof(test_data), 0x12345678));
}

void test_transfer_validate_checksum_null_data(void) {
    TEST_ASSERT_FALSE(transfer_validate_checksum(NULL, sizeof(test_data), test_checksum));
}

void test_transfer_validate_checksum_zero_length(void) {
    TEST_ASSERT_FALSE(transfer_validate_checksum(test_data, 0, test_checksum));
}

// Test sequence validation
void test_transfer_validate_sequence_initial(void) {
    g_context->last_sequence = 0;
    TEST_ASSERT_TRUE(transfer_validate_sequence(1));
}

void test_transfer_validate_sequence_wraparound(void) {
    g_context->last_sequence = 255;
    TEST_ASSERT_TRUE(transfer_validate_sequence(0));
}

void test_transfer_validate_sequence_invalid(void) {
    g_context->last_sequence = 5;
    TEST_ASSERT_FALSE(transfer_validate_sequence(7));
}

void test_transfer_validate_sequence_duplicate(void) {
    g_context->last_sequence = 5;
    TEST_ASSERT_FALSE(transfer_validate_sequence(5));
}

// Test buffer validation
void test_transfer_buffer_available_initial(void) {
    TEST_ASSERT_FALSE(transfer_buffer_available());
}

void test_transfer_buffer_available_after_allocation(void) {
    transfer_allocate_buffer(1024);
    TEST_ASSERT_TRUE(transfer_buffer_available());
}

void test_transfer_buffer_available_after_free(void) {
    transfer_allocate_buffer(1024);
    transfer_free_buffer();
    TEST_ASSERT_FALSE(transfer_buffer_available());
}

// Test chunk validation
void test_transfer_validate_chunk_valid(void) {
    Packet packet;
    packet.header.packet_type = PACKET_TYPE_DATA;
    packet.header.sequence = 1;
    packet.header.length = sizeof(test_data);
    memcpy(packet.payload, test_data, sizeof(test_data));
    packet.header.checksum = test_checksum;
    
    g_context->last_sequence = 0;
    TEST_ASSERT_TRUE(transfer_validate_chunk(&packet));
}

void test_transfer_validate_chunk_invalid_type(void) {
    Packet packet;
    packet.header.packet_type = PACKET_TYPE_SYNC;  // Wrong type
    packet.header.sequence = 1;
    packet.header.length = sizeof(test_data);
    memcpy(packet.payload, test_data, sizeof(test_data));
    packet.header.checksum = test_checksum;
    
    g_context->last_sequence = 0;
    TEST_ASSERT_FALSE(transfer_validate_chunk(&packet));
}

void test_transfer_validate_chunk_invalid_sequence(void) {
    Packet packet;
    packet.header.packet_type = PACKET_TYPE_DATA;
    packet.header.sequence = 2;  // Wrong sequence
    packet.header.length = sizeof(test_data);
    memcpy(packet.payload, test_data, sizeof(test_data));
    packet.header.checksum = test_checksum;
    
    g_context->last_sequence = 0;
    TEST_ASSERT_FALSE(transfer_validate_chunk(&packet));
}

void test_transfer_validate_chunk_invalid_checksum(void) {
    Packet packet;
    packet.header.packet_type = PACKET_TYPE_DATA;
    packet.header.sequence = 1;
    packet.header.length = sizeof(test_data);
    memcpy(packet.payload, test_data, sizeof(test_data));
    packet.header.checksum = 0x12345678;  // Wrong checksum
    
    g_context->last_sequence = 0;
    TEST_ASSERT_FALSE(transfer_validate_chunk(&packet));
}

// Test sequence validation state checks
void test_transfer_sequence_valid_in_progress(void) {
    g_context->state = TRANSFER_STATE_IN_PROGRESS;
    g_context->chunks_received = 5;
    g_context->chunks_expected = 10;
    TEST_ASSERT_TRUE(transfer_sequence_valid());
}

void test_transfer_sequence_valid_starting(void) {
    g_context->state = TRANSFER_STATE_STARTING;
    g_context->chunks_received = 0;
    g_context->chunks_expected = 10;
    TEST_ASSERT_TRUE(transfer_sequence_valid());
}

void test_transfer_sequence_valid_wrong_state(void) {
    g_context->state = TRANSFER_STATE_IDLE;
    g_context->chunks_received = 5;
    g_context->chunks_expected = 10;
    TEST_ASSERT_FALSE(transfer_sequence_valid());
}

void test_transfer_sequence_valid_all_chunks_received(void) {
    g_context->state = TRANSFER_STATE_IN_PROGRESS;
    g_context->chunks_received = 10;
    g_context->chunks_expected = 10;
    TEST_ASSERT_FALSE(transfer_sequence_valid());
}

// Test full buffer checksum validation
void test_transfer_checksum_valid_empty_buffer(void) {
    TEST_ASSERT_FALSE(transfer_checksum_valid());
}

void test_transfer_checksum_valid_full_buffer(void) {
    // Allocate and fill buffer with test data
    transfer_allocate_buffer(sizeof(test_data));
    memcpy(g_context->buffer, test_data, sizeof(test_data));
    g_context->buffer_offset = sizeof(test_data);
    g_context->last_checksum = test_checksum;
    
    TEST_ASSERT_TRUE(transfer_checksum_valid());
}

void test_transfer_checksum_valid_modified_buffer(void) {
    // Allocate and fill buffer with test data
    transfer_allocate_buffer(sizeof(test_data));
    memcpy(g_context->buffer, test_data, sizeof(test_data));
    g_context->buffer_offset = sizeof(test_data);
    g_context->last_checksum = test_checksum;
    
    // Modify one byte
    g_context->buffer[2] = 0xFF;
    
    TEST_ASSERT_FALSE(transfer_checksum_valid());
}

int main(void) {
    UNITY_BEGIN();
    
    // Checksum validation tests
    RUN_TEST(test_transfer_validate_checksum_valid);
    RUN_TEST(test_transfer_validate_checksum_invalid);
    RUN_TEST(test_transfer_validate_checksum_null_data);
    RUN_TEST(test_transfer_validate_checksum_zero_length);
    
    // Sequence validation tests
    RUN_TEST(test_transfer_validate_sequence_initial);
    RUN_TEST(test_transfer_validate_sequence_wraparound);
    RUN_TEST(test_transfer_validate_sequence_invalid);
    RUN_TEST(test_transfer_validate_sequence_duplicate);
    
    // Buffer validation tests
    RUN_TEST(test_transfer_buffer_available_initial);
    RUN_TEST(test_transfer_buffer_available_after_allocation);
    RUN_TEST(test_transfer_buffer_available_after_free);
    
    // Chunk validation tests
    RUN_TEST(test_transfer_validate_chunk_valid);
    RUN_TEST(test_transfer_validate_chunk_invalid_type);
    RUN_TEST(test_transfer_validate_chunk_invalid_sequence);
    RUN_TEST(test_transfer_validate_chunk_invalid_checksum);
    
    // Higher-level sequence validation tests
    RUN_TEST(test_transfer_sequence_valid_in_progress);
    RUN_TEST(test_transfer_sequence_valid_starting);
    RUN_TEST(test_transfer_sequence_valid_wrong_state);
    RUN_TEST(test_transfer_sequence_valid_all_chunks_received);
    
    // Full buffer checksum validation tests
    RUN_TEST(test_transfer_checksum_valid_empty_buffer);
    RUN_TEST(test_transfer_checksum_valid_full_buffer);
    RUN_TEST(test_transfer_checksum_valid_modified_buffer);
    
    return UNITY_END();
} 