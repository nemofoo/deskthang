#include "packet.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>  // Add this for printf/snprintf

// Debug packet payload structure - Move this to the top
typedef struct {
    char module[32];
    char message[256];
} __attribute__((packed)) DebugPayload;

// Static packet buffer
static uint8_t g_packet_buffer[MAX_PACKET_SIZE];
static uint16_t g_buffer_size = MAX_PACKET_SIZE;

// Initialize packet buffer
bool packet_buffer_init(void) {
    memset(g_packet_buffer, 0, g_buffer_size);
    return true;
}

// Reset packet buffer
void packet_buffer_reset(void) {
    memset(g_packet_buffer, 0, g_buffer_size);
}

// Get packet buffer
uint8_t *packet_get_buffer(void) {
    return g_packet_buffer;
}

// Get buffer size
uint16_t packet_get_buffer_size(void) {
    return g_buffer_size;
}

// Create a packet
bool packet_create(Packet *packet, PacketType type, const uint8_t *payload, uint16_t length) {
    if (!packet || length > MAX_PACKET_SIZE) {
        return false;
    }
    
    // Set header fields
    packet->header.type = type;
    packet->header.sequence = packet_next_sequence();
    packet->header.length = length;
    
    // Copy payload if present
    if (payload && length > 0) {
        memcpy(packet->payload, payload, length);
    }
    
    // Calculate checksum
    packet->header.checksum = packet_calculate_checksum(packet->payload, length);
    
    return true;
}

// Create specific packet types
bool packet_create_sync(Packet *packet) {
    uint8_t payload = PROTOCOL_VERSION;
    return packet_create(packet, PACKET_TYPE_SYNC, &payload, 1);
}

bool packet_create_sync_ack(Packet *packet) {
    uint8_t payload = PROTOCOL_VERSION;
    return packet_create(packet, PACKET_TYPE_SYNC_ACK, &payload, 1);
}

bool packet_create_ack(Packet *packet, uint8_t sequence) {
    packet->header.type = PACKET_TYPE_ACK;
    packet->header.sequence = sequence;
    packet->header.length = 0;
    packet->header.checksum = 0;
    return true;
}

bool packet_create_nack(Packet *packet, uint8_t sequence) {
    packet->header.type = PACKET_TYPE_NACK;
    packet->header.sequence = sequence;
    packet->header.length = 0;
    packet->header.checksum = 0;
    return true;
}

// Parse raw data into packet
bool packet_parse(const uint8_t *data, uint16_t length, Packet *packet) {
    if (!data || !packet || length < HEADER_SIZE) {
        return false;
    }
    
    // Copy header
    memcpy(&packet->header, data, HEADER_SIZE);
    
    // Validate header fields
    if (!packet_validate(packet)) {
        return false;
    }
    
    // Copy payload if present
    if (packet->header.length > 0) {
        if (length < HEADER_SIZE + packet->header.length) {
            return false;
        }
        memcpy(packet->payload, data + HEADER_SIZE, packet->header.length);
    }
    
    return true;
}

// Validate packet
bool packet_validate(const Packet *packet) {
    if (!packet) {
        logging_write("Protocol", "Null packet received");
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
            break;
        default:
            logging_write_with_context("Protocol", 
                                     "Invalid packet type",
                                     packet_type_to_string(packet->header.type));
            return false;
    }
    
    // Validate protocol version (for SYNC packets)
    if (packet->header.type == PACKET_TYPE_SYNC) {
        uint8_t version = packet->payload[0];
        if (version != PROTOCOL_VERSION) {
            char context[64];
            snprintf(context, sizeof(context), 
                    "Expected v%d, got v%d", 
                    PROTOCOL_VERSION, version);
            logging_write_with_context("Protocol", "Version mismatch", context);
            return false;
        }
    }
    
    // Validate length
    if (packet->header.length > MAX_PACKET_SIZE) {
        return false;
    }
    
    // Validate sequence number
    if (!packet_validate_sequence(packet->header.sequence)) {
        return false;
    }
    
    // Validate checksum if payload present
    if (packet->header.length > 0) {
        if (!packet_verify_checksum(packet)) {
            return false;
        }
    }
    
    return true;
}

// Packet field access
PacketType packet_get_type(const Packet *packet) {
    return (PacketType)packet->header.type;
}

uint8_t packet_get_sequence(const Packet *packet) {
    return packet->header.sequence;
}

uint16_t packet_get_length(const Packet *packet) {
    return packet->header.length;
}

uint32_t packet_get_checksum(const Packet *packet) {
    return packet->header.checksum;
}

const uint8_t *packet_get_payload(const Packet *packet) {
    return packet->payload;
}

// Checksum calculation
uint32_t packet_calculate_checksum(const uint8_t *data, uint16_t length) {
    // TODO: Implement CRC32 calculation
    // For now, return simple sum of bytes
    uint32_t sum = 0;
    for (uint16_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;
}

bool packet_verify_checksum(const Packet *packet) {
    if (!packet) {
        return false;
    }
    
    uint32_t calculated = packet_calculate_checksum(packet->payload, packet->header.length);
    return calculated == packet->header.checksum;
}

// Sequence number management
static uint8_t g_sequence = 0;

uint8_t packet_next_sequence(void) {
    return ++g_sequence;
}

bool packet_validate_sequence(uint8_t sequence) {
    // Allow sequence numbers to wrap around
    return (sequence == (g_sequence + 1)) || 
           (g_sequence == 255 && sequence == 0);
}

// Packet transmission
bool packet_transmit(const Packet *packet) {
    if (!packet) {
        return false;
    }
    
    // TODO: Implement actual transmission over hardware interface
    return true;
}

bool packet_receive(Packet *packet) {
    if (!packet) {
        return false;
    }
    
    // TODO: Implement actual reception from hardware interface
    return true;
}

// Debug support
void packet_print(const Packet *packet) {
    if (!packet) {
        return;
    }
    
    printf("Packet:\n");
    printf("  Type: %s (0x%02X)\n", packet_type_to_string(packet_get_type(packet)), packet->header.type);
    printf("  Sequence: %u\n", packet->header.sequence);
    printf("  Length: %u\n", packet->header.length);
    printf("  Checksum: 0x%08X\n", packet->header.checksum);
    
    if (packet->header.length > 0) {
        if (packet->header.type == PACKET_TYPE_DEBUG) {
            DebugPayload *debug = (DebugPayload*)packet->payload;
            printf("  Debug Info:\n");
            printf("    Module: %s\n", debug->module);
            printf("    Message: %s\n", debug->message);
        } else {
            printf("  Payload: ");
            for (uint16_t i = 0; i < packet->header.length; i++) {
                printf("%02X ", packet->payload[i]);
            }
            printf("\n");
        }
    }
}

const char *packet_type_to_string(PacketType type) {
    switch (type) {
        case PACKET_TYPE_SYNC:     return "SYNC";
        case PACKET_TYPE_SYNC_ACK: return "SYNC_ACK";
        case PACKET_TYPE_CMD:      return "CMD";
        case PACKET_TYPE_DATA:     return "DATA";
        case PACKET_TYPE_ACK:      return "ACK";
        case PACKET_TYPE_NACK:     return "NACK";
        case PACKET_TYPE_DEBUG:    return "DEBUG";
        default:              return "UNKNOWN";
    }
}

bool packet_create_debug(Packet *packet, const char *module, const char *message) {
    if (!packet || !module || !message) {
        return false;
    }
    
    DebugPayload payload;
    
    // Copy strings with length limits to prevent buffer overflow
    strncpy(payload.module, module, sizeof(payload.module) - 1);
    payload.module[sizeof(payload.module) - 1] = '\0';
    
    strncpy(payload.message, message, sizeof(payload.message) - 1);
    payload.message[sizeof(payload.message) - 1] = '\0';
    
    return packet_create(packet, PACKET_TYPE_DEBUG, (uint8_t*)&payload, sizeof(DebugPayload));
}
