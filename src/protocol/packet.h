#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol_constants.h"  // For MAX_PACKET_SIZE
#include "../error/logging.h"

// Packet types
typedef enum {
    PACKET_SYNC = 0x1B,      // Synchronization request
    PACKET_SYNC_ACK = 0x1C,  // Synchronization acknowledgment
    PACKET_CMD = 0x1D,       // Command packet
    PACKET_DATA = 0x1E,      // Data packet
    PACKET_ACK = 0x1F,       // Acknowledgment
    PACKET_NACK = 0x20,      // Negative acknowledgment
    PACKET_DEBUG = 0x21      // Debug message packet
} PacketType;

// Packet header structure (8 bytes)
typedef struct {
    uint8_t type;            // Packet type
    uint8_t sequence;        // Sequence number
    uint16_t length;         // Payload length
    uint32_t checksum;       // CRC32 checksum
} __attribute__((packed)) PacketHeader;

// Complete packet structure
typedef struct {
    PacketHeader header;             // 8-byte header
    uint8_t payload[MAX_PACKET_SIZE]; // Variable length payload (max 512 bytes)
} Packet;

// Packet buffer management
bool packet_buffer_init(void);
void packet_buffer_reset(void);
uint8_t *packet_get_buffer(void);
uint16_t packet_get_buffer_size(void);

// Packet creation
bool packet_create(Packet *packet, PacketType type, const uint8_t *payload, uint16_t length);
bool packet_create_sync(Packet *packet);
bool packet_create_sync_ack(Packet *packet);
bool packet_create_ack(Packet *packet, uint8_t sequence);
bool packet_create_nack(Packet *packet, uint8_t sequence);
bool packet_create_debug(Packet *packet, const char *module, const char *message);

// Packet parsing
bool packet_parse(const uint8_t *data, uint16_t length, Packet *packet);
bool packet_validate(const Packet *packet);

// Packet fields access
PacketType packet_get_type(const Packet *packet);
uint8_t packet_get_sequence(const Packet *packet);
uint16_t packet_get_length(const Packet *packet);
uint32_t packet_get_checksum(const Packet *packet);
const uint8_t *packet_get_payload(const Packet *packet);

// Checksum calculation
uint32_t packet_calculate_checksum(const uint8_t *data, uint16_t length);
bool packet_verify_checksum(const Packet *packet);

// Sequence number management
uint8_t packet_next_sequence(void);
bool packet_validate_sequence(uint8_t sequence);

// Packet transmission
bool packet_transmit(const Packet *packet);
bool packet_receive(Packet *packet);

// Debug support
void packet_print(const Packet *packet);
const char *packet_type_to_string(PacketType type);

#endif // PACKET_H
