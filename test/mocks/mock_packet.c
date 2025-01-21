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

    // Mock packet validation based on type
    switch (packet->header.packet_type) {
        case PACKET_TYPE_SYNC:
        case PACKET_TYPE_SYNC_ACK:
        case PACKET_TYPE_COMMAND:
        case PACKET_TYPE_DATA:
        case PACKET_TYPE_CONTROL:
            return true;
        default:
            return false;
    }
} 