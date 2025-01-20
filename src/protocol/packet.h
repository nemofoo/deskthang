#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include "../common/deskthang_constants.h"
#include "../error/logging.h"

// Add packet-specific constants to deskthang_constants.h first
// Then update packet.h to use them

typedef enum {
    PACKET_TYPE_SYNC = PACKET_SYNC_BYTE,
    PACKET_TYPE_SYNC_ACK = PACKET_SYNC_ACK_BYTE,
    PACKET_TYPE_CMD = PACKET_CMD_BYTE,
    PACKET_TYPE_DATA = PACKET_DATA_BYTE,
    PACKET_TYPE_ACK = PACKET_ACK_BYTE,
    PACKET_TYPE_NACK = PACKET_NACK_BYTE,
    PACKET_TYPE_DEBUG = PACKET_DEBUG_BYTE
} PacketType;

/**
 * Packet header structure.
 * Fixed size of HEADER_SIZE (8) bytes, containing:
 * - type (1 byte): One of the PACKET_TYPE_* constants
 * - sequence (1 byte): Packet sequence number for ordering
 * - length (2 bytes): Length of payload data
 * - checksum (4 bytes): CRC32 of payload data
 */
typedef struct {
    PacketType type;            // Packet type
    uint8_t sequence;        // Sequence number
    uint16_t length;         // Payload length
    uint32_t checksum;       // CRC32 checksum
} __attribute__((packed)) PacketHeader;

/**
 * Complete packet structure.
 * Contains a fixed-size header followed by variable-length payload.
 * Maximum total packet size is MAX_PACKET_SIZE bytes.
 * Payload data is transferred in CHUNK_SIZE chunks during data transfer.
 */
typedef struct {
    PacketHeader header;                  // Fixed HEADER_SIZE (8) byte header
    uint8_t payload[MAX_PACKET_SIZE - HEADER_SIZE];     // Variable length payload
} Packet;

// Debug packet payload structure
typedef struct {
    uint32_t timestamp;
    char module[DEBUG_MODULE_NAME_MAX];
    char message[DEBUG_MESSAGE_MAX];
} __attribute__((packed)) DebugPayload;

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
