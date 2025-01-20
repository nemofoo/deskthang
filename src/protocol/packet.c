#include "packet.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>  // Add this for printf/snprintf
#include "../common/deskthang_constants.h"
#include "../hardware/serial.h"  // Add this for serial functions
#include "../system/time.h"     // Add this for time functions

// Static packet buffer
static uint8_t g_packet_buffer[MAX_PACKET_SIZE];
static uint16_t g_buffer_size = MAX_PACKET_SIZE;

// Static frame markers
static const uint8_t frame_start = FRAME_START;
static const uint8_t frame_end = FRAME_END;
static const uint8_t frame_escape = FRAME_ESCAPE;

// CRC32 lookup table
static const uint32_t crc32_table[] = {
 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D 
};

// Packet transmission statistics
static struct {
    uint32_t packets_sent;
    uint32_t packets_failed;
    uint32_t bytes_transmitted;
    uint32_t last_transmit_time;
    uint32_t transmission_errors;
} transmission_stats = {0};

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

// Error flags for NACK packets
#define NACK_ERROR_INVALID_TYPE     0x01
#define NACK_ERROR_VERSION_MISMATCH 0x02
#define NACK_ERROR_CHECKSUM        0x04
#define NACK_ERROR_SEQUENCE        0x08
#define NACK_ERROR_LENGTH          0x10
#define NACK_ERROR_OVERFLOW        0x20
#define NACK_ERROR_TRANSMISSION    0x40

bool packet_create_nack(Packet *packet, uint8_t sequence) {
    if (!packet) {
        return false;
    }
    
    ErrorDetails *error = error_get_last();
    if (error) {
        // Map error codes to NACK flags
        uint8_t error_flags = 0;
        switch (error->code) {
            case ERROR_PROTOCOL_INVALID_TYPE:
                error_flags |= NACK_ERROR_INVALID_TYPE;
                break;
            case ERROR_PROTOCOL_VERSION_MISMATCH:
                error_flags |= NACK_ERROR_VERSION_MISMATCH;
                break;
            case ERROR_PROTOCOL_CHECKSUM:
                error_flags |= NACK_ERROR_CHECKSUM;
                break;
            case ERROR_PROTOCOL_SEQUENCE:
                error_flags |= NACK_ERROR_SEQUENCE;
                break;
            case ERROR_PROTOCOL_OVERFLOW:
                error_flags |= NACK_ERROR_OVERFLOW;
                break;
            case ERROR_PROTOCOL_TRANSMISSION:
                error_flags |= NACK_ERROR_TRANSMISSION;
                break;
            default:
                break;
        }
        
        packet->header.type = PACKET_TYPE_NACK;
        packet->header.sequence = sequence;
        packet->header.length = sizeof(ErrorDetails);
        memcpy(packet->payload, error, sizeof(ErrorDetails));
        
        // Calculate checksum over the NACK payload
        packet->header.checksum = packet_calculate_checksum(packet->payload, packet->header.length);
    }
    
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
    if (!data) return 0;
    
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint16_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ byte];
    }
    
    return crc ^ 0xFFFFFFFF;
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

// Transmit a single byte with escape sequence if needed
static bool transmit_byte(uint8_t byte) {
    // Check if byte needs escaping
    if (byte == FRAME_START || byte == FRAME_END || byte == FRAME_ESCAPE) {
        uint8_t escape_sequence[2] = {FRAME_ESCAPE, byte ^ 0x20};
        return serial_write(escape_sequence, 2);
    }
    return serial_write(&byte, 1);
}

// Transmit data with framing
static bool transmit_framed_data(const uint8_t* data, uint16_t length) {
    // Send frame start
    if (!serial_write(&frame_start, 1)) {
        return false;
    }
    
    // Send data with escape sequences
    for (uint16_t i = 0; i < length; i++) {
        if (!transmit_byte(data[i])) {
            return false;
        }
    }
    
    // Send frame end
    if (!serial_write(&frame_end, 1)) {
        return false;
    }
    
    return true;
}

// Create a packet transmission report
static void update_transmission_stats(bool success, uint16_t bytes_sent) {
    transmission_stats.last_transmit_time = deskthang_time_get_ms();
    if (success) {
        transmission_stats.packets_sent++;
        transmission_stats.bytes_transmitted += bytes_sent;
    } else {
        transmission_stats.packets_failed++;
        transmission_stats.transmission_errors++;
    }
}

// Get transmission statistics
bool packet_get_transmission_stats(PacketTransmissionStats* stats) {
    if (!stats) {
        return false;
    }
    
    stats->packets_sent = transmission_stats.packets_sent;
    stats->packets_failed = transmission_stats.packets_failed;
    stats->bytes_transmitted = transmission_stats.bytes_transmitted;
    stats->last_transmit_time = transmission_stats.last_transmit_time;
    stats->transmission_errors = transmission_stats.transmission_errors;
    
    return true;
}

// Enhanced packet transmission with verification
bool packet_transmit(const Packet* packet) {
    if (!packet) {
        logging_write("Protocol", "Null packet in transmission");
        return false;
    }
    
    // Calculate total packet size
    uint16_t total_size = HEADER_SIZE + packet->header.length;
    bool success = false;
    uint32_t retry_count = 0;
    const uint32_t max_retries = 3;
    
    while (!success && retry_count < max_retries) {
        // First send header with framing
        if (!transmit_framed_data((const uint8_t*)&packet->header, HEADER_SIZE)) {
            char context[32];
            snprintf(context, sizeof(context), "Retry %u/%u", 
                (unsigned)retry_count + 1, (unsigned)max_retries);
            logging_write_with_context("Protocol", "Header transmission failed", context);
            retry_count++;
            serial_flush();
            deskthang_delay_ms(5 * retry_count);  // Exponential backoff
            continue;
        }
        
        // Then send payload if present
        if (packet->header.length > 0) {
            if (!transmit_framed_data(packet->payload, packet->header.length)) {
                char context[32];
                snprintf(context, sizeof(context), "Retry %u/%u", 
                    (unsigned)retry_count + 1, (unsigned)max_retries);
                logging_write_with_context("Protocol", "Payload transmission failed", context);
                retry_count++;
                serial_flush();
                deskthang_delay_ms(5 * retry_count);  // Exponential backoff
                continue;
            }
        }
        
        // Ensure everything is sent
        serial_flush();
        success = true;
    }
    
    // Update transmission statistics
    update_transmission_stats(success, total_size);
    
    if (!success) {
        ErrorDetails error = {
            .type = ERROR_TYPE_PROTOCOL,
            .severity = ERROR_SEVERITY_ERROR,
            .code = ERROR_PROTOCOL_TRANSMISSION,
            .timestamp = deskthang_time_get_ms(),
            .source_state = 0,
            .recoverable = true,
            .retry_count = retry_count,
            .backoff_ms = 5 * retry_count
        };
        strncpy(error.message, "Packet transmission failed", ERROR_MESSAGE_SIZE - 1);
        snprintf(error.context, ERROR_CONTEXT_SIZE - 1, 
                "Type: %s, Size: %u", 
                packet_type_to_string(packet->header.type), 
                total_size);
        logging_error(&error);
    }
    
    return success;
}

// Enhanced packet reception with framing
bool packet_receive(Packet* packet) {
    if (!packet) {
        return false;
    }
    
    uint8_t byte;
    bool in_frame = false;
    bool escaped = false;
    uint16_t data_index = 0;
    uint8_t receive_buffer[MAX_PACKET_SIZE];
    
    // Wait for start of frame
    while (serial_read(&byte, 1)) {
        if (byte == FRAME_START) {
            in_frame = true;
            break;
        }
    }
    
    if (!in_frame) {
        logging_write("Protocol", "Frame start not found");
        return false;
    }
    
    // Read until frame end
    while (serial_read(&byte, 1)) {
        if (escaped) {
            receive_buffer[data_index++] = byte ^ 0x20;
            escaped = false;
        } else if (byte == FRAME_ESCAPE) {
            escaped = true;
        } else if (byte == FRAME_END) {
            break;
        } else if (byte == FRAME_START) {
            // Unexpected frame start - reset
            data_index = 0;
            continue;
        } else {
            receive_buffer[data_index++] = byte;
        }
        
        if (data_index >= MAX_PACKET_SIZE) {
            logging_write("Protocol", "Packet size exceeds maximum");
            return false;
        }
    }
    
    // Check if we have at least a header
    if (data_index < HEADER_SIZE) {
        logging_write("Protocol", "Incomplete packet header");
        return false;
    }
    
    // Copy header
    memcpy(&packet->header, receive_buffer, HEADER_SIZE);
    
    // Validate header
    if (!packet_validate_header(&packet->header)) {
        logging_write("Protocol", "Invalid packet header");
        return false;
    }
    
    // Copy payload if present
    if (packet->header.length > 0) {
        if (data_index < HEADER_SIZE + packet->header.length) {
            logging_write("Protocol", "Incomplete packet payload");
            return false;
        }
        memcpy(packet->payload, receive_buffer + HEADER_SIZE, packet->header.length);
    }
    
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

// Add header validation
bool packet_validate_header(const PacketHeader *header) {
    if (!header) {
        return false;
    }
    
    // Validate packet type
    switch (header->type) {
        case PACKET_TYPE_SYNC:
        case PACKET_TYPE_SYNC_ACK:
        case PACKET_TYPE_CMD:
        case PACKET_TYPE_DATA:
        case PACKET_TYPE_ACK:
        case PACKET_TYPE_NACK:
        case PACKET_TYPE_DEBUG:
            break;
        default:
            return false;
    }
    
    // Validate length
    if (header->length > MAX_PACKET_SIZE - HEADER_SIZE) {
        return false;
    }
    
    // Validate sequence
    if (!packet_validate_sequence(header->sequence)) {
        return false;
    }
    
    return true;
}

bool packet_handle_nack(const Packet *packet) {
    if (!packet || packet->header.length < sizeof(ErrorDetails)) {
        return false;
    }
    
    const ErrorDetails *error = (const ErrorDetails *)packet->payload;
    char context[64];
    
    // Log the NACK details
    snprintf(context, sizeof(context), "Flags: 0x%02X, Type: %s", 
             error->code,
             packet_type_to_string(error->type));
             
    logging_write_with_context("Protocol", "Received NACK", context);
    
    // Create detailed error report
    ErrorDetails detailed_error = {
        .type = ERROR_TYPE_PROTOCOL,
        .severity = ERROR_SEVERITY_ERROR,
        .code = ERROR_PROTOCOL_NACK_RECEIVED,
        .timestamp = deskthang_time_get_ms(),
        .source_state = 0,
        .recoverable = true
    };
    
    strncpy(detailed_error.message, "NACK received", ERROR_MESSAGE_SIZE - 1);
    strncpy(detailed_error.context, error->context, ERROR_CONTEXT_SIZE - 1);
    
    logging_error(&detailed_error);
    
    return true;
}
