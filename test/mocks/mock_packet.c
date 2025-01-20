#include "mock_packet.h"

bool packet_buffer_init(void) {
    return true;
}

void packet_buffer_reset(void) {
    // Nothing to do in mock
}

bool packet_validate(const Packet *packet) {
    if (!packet) {
        return false;
    }

    // Validate packet type
    switch (packet->header.type) {
        case PACKET_TYPE_SYNC:
        case PACKET_TYPE_SYNC_ACK:
        case PACKET_TYPE_CMD:
        case PACKET_TYPE_DATA:
        case PACKET_TYPE_ACK:
        case PACKET_TYPE_NACK:
        case PACKET_TYPE_DEBUG:
            return true;
        default:
            return false;
    }
} 