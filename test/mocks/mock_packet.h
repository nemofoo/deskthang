#ifndef MOCK_PACKET_H
#define MOCK_PACKET_H

#include <stdint.h>
#include <stdbool.h>

// Mock packet types
typedef enum {
    PACKET_TYPE_SYNC = 0x01,
    PACKET_TYPE_SYNC_ACK = 0x02,
    PACKET_TYPE_CMD = 0x03,
    PACKET_TYPE_DATA = 0x04,
    PACKET_TYPE_ACK = 0x05,
    PACKET_TYPE_NACK = 0x06,
    PACKET_TYPE_DEBUG = 0x07
} PacketType;

// Mock packet header
typedef struct {
    PacketType type;
    uint8_t sequence;
    uint16_t length;
    uint32_t checksum;
} PacketHeader;

// Mock packet
typedef struct {
    PacketHeader header;
    uint8_t payload[256];  // Simplified for testing
} Packet;

// Mock functions
bool packet_buffer_init(void);
void packet_buffer_reset(void);
bool packet_validate(const Packet *packet);

#endif // MOCK_PACKET_H 