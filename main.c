#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/stdio/driver.h"
#include "pico/stdio_usb.h"
#include "GC9A01.h"
#include "colors.h"

// Hardware configuration
#define SPI_BAUD 10000000  // 10MHz

// Display configuration
#define IMAGE_SIZE (240 * 240 * 2)  // 240x240 pixels, 2 bytes per pixel (RGB565)

// Protocol version and configuration
#define PROTOCOL_VERSION 1
#define SERIAL_TIMEOUT_US 2000000    // 2 seconds base timeout
#define SERIAL_DELAY_MS 20           // Delay between operations
#define CHUNK_SIZE 256               // Smaller chunks for reliability
#define BUFFER_SIZE 512              // General purpose buffer size

// Legacy protocol characters (for backward compatibility)
#define ACK_CHAR    'A'             // Acknowledgment character
#define IMAGE_CMD   'I'             // Image command
#define HELP_CMD    'H'             // Help command
#define END_MARKER  'E'             // End marker

// New packet types
#define PKT_SYNC    0x1B            // Start sync sequence
#define PKT_SYNCACK 0x1C            // Sync acknowledgment
#define PKT_CMD     0x1D            // Command packet
#define PKT_DATA    0x1E            // Data packet
#define PKT_ACK     0x1F            // Acknowledgment
#define PKT_NACK    0x20            // Negative acknowledgment

// New command types (for packet-based protocol)
#define CMD_IMAGE   IMAGE_CMD       // Image command (same as legacy)
#define CMD_HELP    HELP_CMD        // Help command (same as legacy)
#define CMD_END     END_MARKER      // End marker (same as legacy)

// Packet header structure (8 bytes)
struct packet_header {
    uint8_t type;                   // Packet type
    uint8_t sequence;               // Sequence number
    uint16_t length;                // Payload length
    uint32_t checksum;              // CRC32 of payload
};

// CRC32 lookup table
static const uint32_t crc_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

// Protocol helper functions
uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc = crc_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

// Read exactly n bytes with timeout
bool read_exact(uint8_t *buffer, size_t n, uint32_t timeout_us) {
    size_t received = 0;
    absolute_time_t timeout = make_timeout_time_us(timeout_us);

    printf("Reading %d bytes...\n", n);
    fflush(stdout);

    while (received < n) {
        int c = getchar_timeout_us(10000); // Longer polling timeout (10ms)
        if (c == PICO_ERROR_TIMEOUT) {
            if (absolute_time_diff_us(get_absolute_time(), timeout) < 0) {
                printf("Global timeout after %d/%d bytes\n", received, n);
                fflush(stdout);
                return false;
            }
            continue;
        }
        buffer[received++] = (uint8_t)c;
        printf("Got byte %d/%d: 0x%02X\n", received, n, (uint8_t)c);
        fflush(stdout);
    }
    
    printf("Successfully read %d bytes\n", n);
    fflush(stdout);
    return true;
}

// Write all bytes with retry
bool write_all(const uint8_t *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        putchar(data[i]);
    }
    fflush(stdout);
    return true;
}

// Read a complete packet
bool read_packet(struct packet_header *header, uint8_t *payload, size_t max_payload) {
    printf("Attempting to read packet...\n");
    fflush(stdout);

    // Read header
    if (!read_exact((uint8_t*)header, sizeof(*header), SERIAL_TIMEOUT_US)) {
        printf("Failed to read packet header\n");
        fflush(stdout);
        return false;
    }

    printf("Got packet: type=0x%02X, seq=%d, len=%d\n", 
           header->type, header->sequence, header->length);
    fflush(stdout);

    // Validate length
    if (header->length > max_payload) {
        return false;
    }

    // Read payload
    if (header->length > 0) {
        if (!read_exact(payload, header->length, SERIAL_TIMEOUT_US)) {
            return false;
        }

        // Verify checksum
        uint32_t calc_crc = calculate_crc32(payload, header->length);
        if (calc_crc != header->checksum) {
            return false;
        }
    }

    return true;
}

// Write a complete packet
bool write_packet(uint8_t type, uint8_t sequence, const uint8_t *payload, uint16_t length) {
    struct packet_header header = {
        .type = type,
        .sequence = sequence,
        .length = length,
        .checksum = calculate_crc32(payload, length)
    };

    if (!write_all((uint8_t*)&header, sizeof(header))) {
        return false;
    }

    if (length > 0) {
        if (!write_all(payload, length)) {
            return false;
        }
    }

    return true;
}

// Send acknowledgment
void send_ack(uint8_t sequence) {
    write_packet(PKT_ACK, sequence, NULL, 0);
}

// Send negative acknowledgment
void send_nack(uint8_t sequence, const char *message) {
    write_packet(PKT_NACK, sequence, (const uint8_t*)message, strlen(message));
}

// Pin definitions
#define PIN_MOSI 19
#define PIN_SCK  18
#define PIN_CS   17
#define PIN_DC   16
#define PIN_RST  20
#define SPI_PORT spi0

void debug_print_hex(uint8_t byte) {
    printf("Received: 0x%02X\n", byte);
}

// Implementation of the hardware abstraction layer
void GC9A01_set_reset(uint8_t val) {
    gpio_put(PIN_RST, val);
}

void GC9A01_set_data_command(uint8_t val) {
    gpio_put(PIN_DC, val);
}

void GC9A01_set_chip_select(uint8_t val) {
    gpio_put(PIN_CS, val);
}

void GC9A01_spi_tx(uint8_t *data, size_t len) {
    spi_write_blocking(SPI_PORT, data, len);
}

void GC9A01_delay(uint16_t ms) {
    sleep_ms(ms);
}

void display_init(void) {
    // Initialize SPI pins
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    
    gpio_init(PIN_CS);
    gpio_init(PIN_DC);
    gpio_init(PIN_RST);
    
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    
    gpio_put(PIN_CS, 1);
    gpio_put(PIN_DC, 1);
    
    spi_init(SPI_PORT, SPI_BAUD);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    GC9A01_init();
}

// Try to detect protocol version from initial command
bool detect_protocol_version(void) {
    struct packet_header header;
    uint8_t payload[8];
    
    // Try reading as new protocol packet
    if (read_packet(&header, payload, sizeof(payload))) {
        if (header.type == PKT_SYNC && header.length > 0) {
            // Got valid sync packet, acknowledge with same version
            write_packet(PKT_SYNCACK, header.sequence, payload, header.length);
            return true;
        }
    }
    
    return false; // Fall back to legacy protocol
}

void display_raw_image(void) {
    printf("Ready for image\n");
    fflush(stdout);
    
    struct GC9A01_frame frame = {{0,0}, {239,239}};
    GC9A01_set_frame(frame);
    GC9A01_write_command(GC9A01_MEM_WR);
    
    uint8_t buffer[BUFFER_SIZE];
    size_t total_received = 0;
    uint8_t expected_sequence = 0;
    
    // Prepare display
    GC9A01_set_data_command(1);
    GC9A01_set_chip_select(0);
    
    struct packet_header header;
    bool transfer_complete = false;
    
    while (!transfer_complete) {
        if (!read_packet(&header, buffer, sizeof(buffer))) {
            printf("Error: Failed to read packet\n");
            send_nack(expected_sequence, "Read error");
            continue;
        }
        
        switch (header.type) {
            case PKT_DATA:
                if (header.sequence != expected_sequence) {
                    printf("Error: Out of sequence packet (got %d, expected %d)\n", 
                           header.sequence, expected_sequence);
                    send_nack(header.sequence, "Bad sequence");
                    continue;
                }
                
                // Process chunk
                GC9A01_spi_tx(buffer, header.length);
                total_received += header.length;
                
                // Acknowledge
                send_ack(expected_sequence);
                expected_sequence++;
                
                // Progress
                printf("Progress: %d/%d bytes (%.1f%%)\n", 
                       total_received, IMAGE_SIZE,
                       (float)total_received * 100.0f / IMAGE_SIZE);
                break;
                
            case PKT_CMD:
                if (header.length == 1 && buffer[0] == CMD_END) {
                    transfer_complete = true;
                    send_ack(header.sequence);
                }
                break;
                
            default:
                send_nack(header.sequence, "Invalid packet type");
                break;
        }
    }
    
    GC9A01_set_chip_select(1);
    printf("Image transfer complete\n");
    fflush(stdout);
}

void test_pattern_checkerboard(void) {
    printf("Drawing checkerboard pattern...\n");
    fflush(stdout);
    
    printf("Setting display frame...\n");
    fflush(stdout);
    struct GC9A01_frame frame = {{0,0}, {239,239}};
    GC9A01_set_frame(frame);
    
    uint16_t colors[] = {COLOR_BLACK, COLOR_WHITE};
    uint8_t color_bytes[2];
    
    printf("Starting pattern draw\n");
    fflush(stdout);
    
    for(int y = 0; y < 240; y++) {
        for(int x = 0; x < 240; x++) {
            uint16_t current_color = colors[((x/20) + (y/20)) % 2];
            color_bytes[0] = current_color >> 8;
            color_bytes[1] = current_color & 0xFF;
            
            if(x == 0 && y == 0) {
                GC9A01_write(color_bytes, 2);
            } else {
                GC9A01_write_continue(color_bytes, 2);
            }
        }
        if (y % 30 == 0) {
            printf("Progress: %d%%\n", (y * 100) / 240);
            fflush(stdout);
        }
    }
    
    printf("Checkerboard pattern completed\n");
    fflush(stdout);
}

void test_pattern_stripes(void) {
    printf("Drawing stripe pattern...\n");
    fflush(stdout);
    
    printf("Setting display frame...\n");
    fflush(stdout);
    struct GC9A01_frame frame = {{0,0}, {239,239}};
    GC9A01_set_frame(frame);
    
    uint16_t colors[] = {COLOR_RED, COLOR_BLUE};
    uint8_t color_bytes[2];
    
    printf("Starting pattern draw\n");
    fflush(stdout);
    
    // Start the memory write command
    GC9A01_write_command(GC9A01_MEM_WR);
    
    // Set data mode and select chip
    GC9A01_set_data_command(1);
    GC9A01_set_chip_select(0);
    
    for(int y = 0; y < 240; y++) {
        uint16_t current_color = colors[(y/30) % 2];
        color_bytes[0] = current_color >> 8;
        color_bytes[1] = current_color & 0xFF;
        
        // Write entire row of pixels
        for(int x = 0; x < 240; x++) {
            GC9A01_spi_tx(color_bytes, 2);
        }
    }
    
    // Deselect chip when done
    GC9A01_set_chip_select(1);
    
    printf("Stripe pattern completed\n");
    fflush(stdout);
}

void test_pattern_gradient(void) {
    printf("Drawing gradient pattern...\n");
    fflush(stdout);
    
    struct GC9A01_frame frame = {{0,0}, {239,239}};
    GC9A01_set_frame(frame);
    
    uint8_t color_bytes[2];
    
    for(int y = 0; y < 240; y++) {
        for(int x = 0; x < 240; x++) {
            uint16_t current_color = RGB565(x & 0xFF, 0, 0);
            color_bytes[0] = current_color >> 8;
            color_bytes[1] = current_color & 0xFF;
            
            if(x == 0 && y == 0) {
                GC9A01_write(color_bytes, 2);
            } else {
                GC9A01_write_continue(color_bytes, 2);
            }
        }
    }
    
    fflush(stdout);
}

// Function to handle display pattern selection
void print_help(void) {
    printf("\nAvailable Commands:\n");
    printf("------------------\n");
    printf("%c : Upload and display image (240x240 RGB565)\n", IMAGE_CMD);
    printf("1 : Show checkerboard pattern (black/white)\n");
    printf("2 : Show stripe pattern (red/blue)\n");
    printf("3 : Show gradient pattern (red)\n");
    printf("%c : Show this help message\n", HELP_CMD);
    printf("\nAll commands will send ACK on receipt\n");
    fflush(stdout);
}

void set_display_pattern(char pattern) {
    fflush(stdout);
    
    switch(pattern) {
        case IMAGE_CMD: // Image
            display_raw_image();
            break;
        case '1':
            test_pattern_checkerboard();
            break;
        case '2':
            test_pattern_stripes();
            break;
        case '3':
            test_pattern_gradient();
            break;
        case HELP_CMD:
            print_help();
            break;
        default:
            break;
    }
}

// Handle a complete packet-based command
void handle_packet_command(uint8_t sequence, const uint8_t *payload, uint16_t length) {
    if (length != 1) {
        send_nack(sequence, "Invalid command length");
        return;
    }

    char cmd = payload[0];
    set_display_pattern(cmd);
    send_ack(sequence);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("Ready\n");
    display_init();
    test_pattern_checkerboard();
    
    struct packet_header header;
    uint8_t buffer[BUFFER_SIZE];
    bool using_packet_protocol = false;

    while(1) {
        // Try reading as packet protocol first
        if (!using_packet_protocol) {
            using_packet_protocol = detect_protocol_version();
        }

        if (using_packet_protocol) {
            // Packet protocol mode
            if (read_packet(&header, buffer, sizeof(buffer))) {
                switch (header.type) {
                    case PKT_SYNC:
                        // Got a sync packet in the middle? Acknowledge again if needed
                        write_packet(PKT_SYNCACK, header.sequence, buffer, header.length);
                        break;
                    case PKT_CMD:
                        handle_packet_command(header.sequence, buffer, header.length);
                        break;
                    case PKT_DATA:
                        // If you send data packets for images or something else
                        // (Similar to display_raw_image's logic)
                        send_nack(header.sequence, "Data packets only valid during image transfer");
                        break;
                    default:
                        send_nack(header.sequence, "Invalid packet type");
                        break;
                }
            }
        } else {
            // Legacy single-byte protocol
            int c = getchar_timeout_us(100000);
            if (c != PICO_ERROR_TIMEOUT && c >= 32 && c <= 126) {
                // Immediately acknowledge receipt
                putchar(ACK_CHAR);
                fflush(stdout);
                
                // Then handle the command
                if (c == HELP_CMD) {
                    print_help();
                } else {
                    set_display_pattern((char)c);
                }
                
                // Clear any pending input
                while (getchar_timeout_us(1000) != PICO_ERROR_TIMEOUT) {
                    // Discard extra bytes
                }
            }
        }
    }
}
