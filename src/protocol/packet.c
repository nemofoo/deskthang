#include "packet.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>  // Add this for printf/snprintf
#include "../common/deskthang_constants.h"
#include "../hardware/serial.h"  // Add this for serial functions
#include "../system/time.h"     // Add this for time functions

// CRC32 table for checksum calculation
const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

// Protocol markers
#define START_MARKER '~'
#define END_MARKER '\n'
#define ESCAPE_CHAR '\\'

// Static sequence counter
static uint8_t g_sequence = 0;

static bool write_escaped(const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        // Escape special characters
        if (data[i] == START_MARKER || data[i] == END_MARKER || data[i] == ESCAPE_CHAR) {
            uint8_t escape_seq[2];
            escape_seq[0] = ESCAPE_CHAR;
            escape_seq[1] = data[i] ^ 0x20;  // XOR with 0x20 to create escaped version
            if (!serial_write(escape_seq, 2)) {
                return false;
            }
        } else {
            // Write normal character
            if (!serial_write(&data[i], 1)) {
                return false;
            }
        }
    }
    return true;
}

bool packet_init(void) {
    g_sequence = 0;
    return true;
}

void packet_deinit(void) {
    // Nothing to clean up
}

bool packet_create(Packet *packet, PacketType type, uint8_t sequence, const uint8_t *payload, uint16_t length) {
    if (!packet || (length > 0 && !payload)) {
        return false;
    }
    
    if (length > MAX_PAYLOAD_SIZE) {
        return false;
    }
    
    // Initialize header (Start Metadata)
    packet->header.start_marker = START_MARKER;
    packet->header.type = type;
    packet->header.sequence = sequence;
    packet->header.length = length;
    
    // Set payload
    if (length > 0) {
        packet->payload = malloc(length);
        if (!packet->payload) {
            return false;
        }
        memcpy(packet->payload, payload, length);
    } else {
        packet->payload = NULL;
    }
    
    // Set end metadata
    packet->checksum = packet_calculate_checksum(packet);
    packet->end_marker = END_MARKER;
    
    return true;
}

bool packet_create_debug(Packet *packet, const char *module, const char *message) {
    if (!packet || !module || !message) {
        return false;
    }
    
    // Format with 2 spaces after ~ for alignment, consistent with protocol docs
    char payload[MAX_PAYLOAD_SIZE];
    int len = snprintf(payload, sizeof(payload), "  %s: %s", module, message);
    if (len < 0 || len >= sizeof(payload)) {
        return false;
    }
    
    return packet_create(packet, PACKET_TYPE_DEBUG, g_sequence++, (uint8_t*)payload, len);
}

bool packet_create_command(Packet *packet, const char *command) {
    if (!packet || !command) {
        return false;
    }
    
    size_t len = strlen(command);
    if (len > MAX_PAYLOAD_SIZE) {
        return false;
    }
    
    return packet_create(packet, PACKET_TYPE_COMMAND, g_sequence++, (uint8_t*)command, len);
}

bool packet_create_data(Packet *packet, const uint8_t *data, uint16_t length) {
    if (!packet || !data || length == 0) {
        return false;
    }
    
    return packet_create(packet, PACKET_TYPE_DATA, g_sequence++, data, length);
}

bool packet_create_ack(Packet *packet, uint8_t sequence) {
    const char *payload = "OK";
    return packet_create(packet, PACKET_TYPE_ACK, sequence, (uint8_t*)payload, strlen(payload));
}

bool packet_create_nack(const Packet *packet, uint8_t sequence, const char *error) {
    // Create a new packet for the NACK response
    Packet nack_packet;
    memset(&nack_packet, 0, sizeof(Packet));
    
    // Set up the NACK packet header
    nack_packet.header.type = PACKET_TYPE_NACK;
    nack_packet.header.sequence = sequence;
    nack_packet.header.length = strlen(error);
    
    // Copy the error message
    memcpy(nack_packet.payload, error, nack_packet.header.length);
    
    // Calculate checksum and transmit
    nack_packet.checksum = packet_calculate_checksum(&nack_packet);
    return packet_transmit(&nack_packet);
}

bool packet_create_error(Packet *packet, const char *module, const char *error) {
    if (!packet || !module || !error) {
        return false;
    }
    
    char payload[MAX_PAYLOAD_SIZE];
    int len = snprintf(payload, sizeof(payload), "%s: %s", module, error);
    if (len < 0 || len >= sizeof(payload)) {
        return false;
    }
    
    return packet_create(packet, PACKET_TYPE_ERROR, g_sequence++, (uint8_t*)payload, len);
}

bool packet_create_sync(Packet *packet, uint8_t version) {
    return packet_create(packet, PACKET_TYPE_SYNC, g_sequence++, &version, 1);
}

uint32_t packet_calculate_checksum(const Packet *packet) {
    if (!packet) {
        return 0;
    }
    
    // Start with header
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *data = (const uint8_t*)&packet->header;
    for (size_t i = 0; i < sizeof(PacketHeader); i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    // Add payload if present
    if (packet->payload && packet->header.length > 0) {
        for (size_t i = 0; i < packet->header.length; i++) {
            crc = crc32_table[(crc ^ packet->payload[i]) & 0xFF] ^ (crc >> 8);
        }
    }
    
    return crc ^ 0xFFFFFFFF;
}

bool packet_validate(const Packet *packet) {
    if (!packet) {
        return false;
    }
    
    // Validate start/end markers
    if (packet->header.start_marker != START_MARKER ||
        packet->end_marker != END_MARKER) {
        return false;
    }
    
    // Validate length
    if (packet->header.length > MAX_PAYLOAD_SIZE) {
        return false;
    }
    
    // Validate checksum
    uint32_t calculated = packet_calculate_checksum(packet);
    return calculated == packet->checksum;
}

bool packet_transmit(const Packet *packet) {
    if (!packet_validate(packet)) {
        return false;
    }
    
    // Write header with escaping
    if (!write_escaped((uint8_t*)&packet->header, sizeof(PacketHeader))) {
        return false;
    }
    
    // Write payload if present
    if (packet->payload && packet->header.length > 0) {
        if (!write_escaped(packet->payload, packet->header.length)) {
            return false;
        }
    }
    
    // Write space before checksum
    uint8_t space = ' ';
    if (!serial_write(&space, 1)) {
        return false;
    }
    
    // Write checksum as hex
    char checksum_hex[9];
    snprintf(checksum_hex, sizeof(checksum_hex), "%08X", packet->checksum);
    if (!write_escaped((uint8_t*)checksum_hex, 8)) {
        return false;
    }
    
    // Write end marker
    return serial_write(&packet->end_marker, 1);
}

static bool read_with_timeout(uint8_t *byte, uint32_t timeout_ms) {
    uint32_t start = deskthang_time_get_ms();
    
    while (deskthang_time_get_ms() - start < timeout_ms) {
        int result = serial_read_byte();
        if (result >= 0) {
            *byte = (uint8_t)result;
            return true;
        }
    }
    
    return false;
}

static bool read_byte_unescaped(uint8_t *byte, uint32_t timeout_ms) {
    uint8_t raw;
    if (!read_with_timeout(&raw, timeout_ms)) {
        return false;
    }
    
    if (raw == ESCAPE_CHAR) {
        // Read the escaped byte
        uint8_t escaped;
        if (!read_with_timeout(&escaped, timeout_ms)) {
            return false;
        }
        // Un-escape by XORing with 0x20
        *byte = escaped ^ 0x20;
    } else {
        *byte = raw;
    }
    return true;
}

bool packet_receive(Packet *packet) {
    if (!packet) {
        return false;
    }
    
    // Read header with timeout
    uint8_t *header_bytes = (uint8_t*)&packet->header;
    for (size_t i = 0; i < sizeof(PacketHeader); i++) {
        if (!read_byte_unescaped(&header_bytes[i], 10)) {
            return false;
        }
    }
    
    // Validate start marker
    if (packet->header.start_marker != START_MARKER) {
        return false;
    }
    
    // Read payload if present
    if (packet->header.length > 0) {
        packet->payload = malloc(packet->header.length);
        if (!packet->payload) {
            return false;
        }
        
        for (size_t i = 0; i < packet->header.length; i++) {
            if (!read_byte_unescaped(&packet->payload[i], 10)) {
                free(packet->payload);
                return false;
            }
        }
    } else {
        packet->payload = NULL;
    }
    
    // Read space before checksum
    uint8_t space;
    if (!read_byte_unescaped(&space, 10) || space != ' ') {
        if (packet->payload) {
            free(packet->payload);
        }
        return false;
    }
    
    // Read checksum as hex
    char checksum_hex[9];
    for (size_t i = 0; i < 8; i++) {
        if (!read_byte_unescaped((uint8_t*)&checksum_hex[i], 10)) {
            if (packet->payload) {
                free(packet->payload);
            }
            return false;
        }
    }
    checksum_hex[8] = '\0';
    
    // Convert hex string to uint32_t
    if (sscanf(checksum_hex, "%x", &packet->checksum) != 1) {
        if (packet->payload) {
            free(packet->payload);
        }
        return false;
    }
    
    // Read end marker
    if (!read_byte_unescaped(&packet->end_marker, 10)) {
        if (packet->payload) {
            free(packet->payload);
        }
        return false;
    }
    
    // Validate the packet
    if (!packet_validate(packet)) {
        if (packet->payload) {
            free(packet->payload);
        }
        return false;
    }
    
    return true;
}

void packet_free(Packet *packet) {
    if (packet && packet->payload) {
        free(packet->payload);
        packet->payload = NULL;
    }
}

// Add buffer initialization
bool packet_buffer_init(void) {
    // No buffer management needed in new implementation
    return true;
}

// Add getter functions
PacketType packet_get_type(const Packet *packet) {
    if (!packet) {
        return PACKET_TYPE_ERROR;
    }
    return packet->header.type;
}

uint8_t packet_get_sequence(const Packet *packet) {
    if (!packet) {
        return 0;
    }
    return packet->header.sequence;
}

const uint8_t *packet_get_payload(const Packet *packet) {
    if (!packet) {
        return NULL;
    }
    return packet->payload;
}

uint16_t packet_get_length(const Packet *packet) {
    if (!packet) {
        return 0;
    }
    return packet->header.length;
}

uint32_t packet_get_checksum(const Packet *packet) {
    if (!packet) {
        return 0;
    }
    return packet->checksum;
}
