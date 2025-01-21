#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // For size_t
#include "../common/deskthang_constants.h"
#include "../error/logging.h"
#include "../error/error.h"
#include "../system/time.h"  // For deskthang_delay_ms
#include "../hardware/serial.h"

// Add packet-specific constants to deskthang_constants.h first
// Then update packet.h to use them

// Protocol error codes (2000-2999 range)
#define ERROR_PROTOCOL_TRANSMISSION 2001
#define ERROR_PROTOCOL_INVALID_TYPE 2002
#define ERROR_PROTOCOL_VERSION_MISMATCH 2003
#define ERROR_PROTOCOL_CHECKSUM 2004
#define ERROR_PROTOCOL_SEQUENCE 2005
#define ERROR_PROTOCOL_OVERFLOW 2006
#define ERROR_PROTOCOL_NACK_RECEIVED 2007

// CRC32 table for checksum calculation
extern const uint32_t crc32_table[256];

// Message types
typedef enum {
    MESSAGE_TYPE_DEBUG = 'D',
    MESSAGE_TYPE_COMMAND = 'C',
    MESSAGE_TYPE_IMAGE = 'I',
    MESSAGE_TYPE_ACK = 'A',
    MESSAGE_TYPE_NACK = 'N'
} MessageType;

// Maximum sizes
#define MAX_PAYLOAD_SIZE 1024
#define MAX_LINE_SIZE (MAX_PAYLOAD_SIZE + 32)  // Extra space for header + footer
#define MAX_MODULE_NAME 32
#define MAX_MESSAGE_SIZE 256

// Message structure
typedef struct {
    MessageType type;
    uint8_t sequence;
    char *payload;
    uint16_t payload_len;
} Message;

// Packet type definitions
typedef enum {
    PACKET_TYPE_DEBUG,    // Debug and log messages
    PACKET_TYPE_COMMAND,  // Host to device commands
    PACKET_TYPE_DATA,     // Raw image data chunks
    PACKET_TYPE_ACK,      // Positive acknowledgment
    PACKET_TYPE_NACK,     // Negative acknowledgment
    PACKET_TYPE_ERROR,    // System/hardware error reports
    PACKET_TYPE_SYNC      // Protocol synchronization
} PacketType;

// Packet flags
#define PACKET_FLAG_START    0x01  // Start of multi-packet sequence
#define PACKET_FLAG_END      0x02  // End of multi-packet sequence
#define PACKET_FLAG_ERROR    0x04  // Error condition
#define PACKET_FLAG_ACK_REQ  0x08  // Acknowledgment required

// Packet header structure (Start Metadata)
typedef struct {
    uint8_t start_marker;
    PacketType type;
    uint8_t sequence;
    uint16_t length;
} PacketHeader;

// Complete packet structure
typedef struct {
    PacketHeader header;  // Start metadata
    uint8_t *payload;     // Variable length payload
    uint32_t checksum;    // End metadata - checksum
    uint8_t end_marker;   // End metadata - marker
} Packet;

// Debug payload structure (for heartbeat and status messages)
typedef struct {
    char module[32];
    char message[256];
} DebugPayload;

// NACK payload structure
typedef struct {
    uint8_t error_flags;
    uint8_t original_type;
    uint8_t context[14];  // Additional error context
} NackPayload;

// Packet buffer management
bool packet_buffer_init(void);
void packet_buffer_reset(void);
uint8_t *packet_get_buffer(void);
uint16_t packet_get_buffer_size(void);

// Core packet functions
bool packet_init(void);
void packet_deinit(void);

// Packet creation functions
bool packet_create(Packet *packet, PacketType type, uint8_t sequence, const uint8_t *payload, uint16_t length);
bool packet_create_debug(Packet *packet, const char *module, const char *message);
bool packet_create_command(Packet *packet, const char *command);
bool packet_create_data(Packet *packet, const uint8_t *data, uint16_t length);
bool packet_create_ack(Packet *packet, uint8_t sequence);
bool packet_create_nack(const Packet *packet, uint8_t sequence, const char *error);
bool packet_create_error(Packet *packet, const char *module, const char *error);
bool packet_create_sync(Packet *packet, uint8_t version);

// Packet validation
bool packet_validate(const Packet *packet);
uint32_t packet_calculate_checksum(const Packet *packet);

// Packet transmission
bool packet_transmit(const Packet *packet);
bool packet_receive(Packet *packet);

// Packet fields access
PacketType packet_get_type(const Packet *packet);
uint8_t packet_get_sequence(const Packet *packet);
uint16_t packet_get_length(const Packet *packet);
uint32_t packet_get_checksum(const Packet *packet);
const uint8_t *packet_get_payload(const Packet *packet);

// Checksum calculation
bool packet_verify_checksum(const Packet *packet);

// Sequence number management
uint8_t packet_next_sequence(void);
bool packet_validate_sequence(uint8_t sequence);

// Transmission statistics tracking
typedef struct {
    uint32_t packets_sent;        // Total packets successfully sent
    uint32_t packets_failed;      // Total packets that failed to send
    uint32_t bytes_transmitted;   // Total bytes transmitted
    uint32_t last_transmit_time;  // Timestamp of last transmission
    uint32_t transmission_errors; // Count of transmission errors
} PacketTransmissionStats;

// Transmission functions
bool packet_get_transmission_stats(PacketTransmissionStats* stats);

// Debug support
void packet_print(const Packet *packet);
const char *packet_type_to_string(PacketType type);

// Cleanup
void packet_free(Packet *packet);

// Function declarations
bool message_create(Message *msg, MessageType type, uint8_t sequence, const char *payload);
bool message_create_debug(Message *msg, uint8_t sequence, const char *module, const char *text);
bool message_create_ack(Message *msg, uint8_t sequence);
bool message_create_nack(Message *msg, uint8_t sequence, const char *error);

bool message_format(const Message *msg, char *buffer, size_t buffer_size);
bool message_parse(const char *line, Message *msg);
bool message_validate(const Message *msg);

// Transmission functions
bool message_transmit(const Message *msg);
bool message_receive(Message *msg);

// Debug support
void message_print(const Message *msg);

// CRC32 calculation
uint32_t calculate_crc32(const char *data, size_t length);

#endif // PACKET_H
