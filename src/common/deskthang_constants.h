#ifndef DESKTHANG_CONSTANTS_H
#define DESKTHANG_CONSTANTS_H

/**
 * Protocol Constants
 * These values are critical for proper communication between host and device.
 * See protocol_architecture.md for detailed requirements.
 */

// Protocol Version
#define PROTOCOL_VERSION 1

// Frame Markers
#define FRAME_START      0x7E
#define FRAME_END       0x7F
#define FRAME_ESCAPE    0x7D

// Buffer Sizes
#define MAX_PACKET_SIZE 512  // Maximum size of a complete packet including header
#define CHUNK_SIZE 256      // Size of individual data transfer chunks
#define HEADER_SIZE 8       // Size of packet header in bytes

// Timing Constants (milliseconds)
#define BASE_TIMEOUT_MS 1000     // Base timeout for operations
#define MIN_RETRY_DELAY_MS 50    // Minimum delay between retry attempts
#define MAX_RETRY_DELAY_MS 1000  // Maximum delay between retry attempts
#define MAX_RETRIES 8            // Maximum number of retry attempts

/**
 * Error Code Ranges
 * Each subsystem has a dedicated range of error codes.
 * See protocol_architecture.md#error-code-ranges for details.
 */
#define ERROR_CODE_HARDWARE_START 1000
#define ERROR_CODE_HARDWARE_END   1999
#define ERROR_CODE_PROTOCOL_START 2000
#define ERROR_CODE_PROTOCOL_END   2999
#define ERROR_CODE_STATE_START    3000
#define ERROR_CODE_STATE_END      3999
#define ERROR_CODE_COMMAND_START  4000
#define ERROR_CODE_COMMAND_END    4999
#define ERROR_CODE_TRANSFER_START 5000
#define ERROR_CODE_TRANSFER_END   5999
#define ERROR_CODE_SYSTEM_START   6000
#define ERROR_CODE_SYSTEM_END     6999

/**
 * Display Hardware Constants
 * Physical characteristics and configuration of the display.
 */
#define DISPLAY_WIDTH  240
#define DISPLAY_HEIGHT 240
#define DISPLAY_COLOR_DEPTH 16  // 16-bit color (RGB565)

// Display SPI Configuration
#define DISPLAY_SPI_BAUD 10000000  // 10MHz
#define DISPLAY_SPI_PORT 0

// Display GPIO Pins (default configuration)
#define DISPLAY_PIN_MOSI 19
#define DISPLAY_PIN_SCK  18
#define DISPLAY_PIN_CS   17
#define DISPLAY_PIN_DC   16
#define DISPLAY_PIN_RST  20

// Display Timing Parameters
#define DISPLAY_RESET_PULSE_US 10000   // 10ms reset pulse
#define DISPLAY_INIT_DELAY_MS  120     // 120ms init delay
#define DISPLAY_CMD_DELAY_US   10      // 10us command delay

/**
 * Command Processing Constants
 */
#define CMD_MAX_LENGTH 32
#define CMD_MAX_PARAMS 8

/**
 * Transfer Constants
 */
#define TRANSFER_MAX_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT * 2)  // 16-bit color
#define TRANSFER_CHUNK_SIZE CHUNK_SIZE

/**
 * Debug and Logging Constants
 */
#define DEBUG_MODULE_NAME_MAX 32
#define DEBUG_MESSAGE_MAX 256
#define DEBUG_CONTEXT_MAX 128

// Add these orientation constants from display.h
#define ORIENTATION_0   0x18
#define ORIENTATION_90  0x28
#define ORIENTATION_180 0x48
#define ORIENTATION_270 0x88

// Add error message size constants
#define ERROR_MESSAGE_SIZE (MAX_PACKET_SIZE/4)    // 128 bytes
#define ERROR_CONTEXT_SIZE (MAX_PACKET_SIZE/2)    // 256 bytes

// Add serial/logging related constants
#define SERIAL_RX_BUFFER_SIZE MAX_PACKET_SIZE
#define SERIAL_TX_BUFFER_SIZE CHUNK_SIZE
#define LOG_PREFIX "[LOG]"

/**
 * Display Controller (GC9A01) Constants
 */
// Command codes
#define GC9A01_COL_ADDR_SET        0x2A
#define GC9A01_ROW_ADDR_SET        0x2B
#define GC9A01_MEM_WR              0x2C
#define GC9A01_MEM_WR_CONT         0x3C
#define GC9A01_COLOR_MODE          0x3A
#define GC9A01_COLOR_MODE__12_BIT  0x03
#define GC9A01_COLOR_MODE__16_BIT  0x05
#define GC9A01_COLOR_MODE__18_BIT  0x06

/**
 * Packet Protocol Constants
 */
#define PACKET_SYNC_BYTE      'S'
#define PACKET_SYNC_ACK_BYTE  'A'
#define PACKET_CMD_BYTE       'C'
#define PACKET_DATA_BYTE      'D'
#define PACKET_ACK_BYTE       'K'
#define PACKET_NACK_BYTE      'N'
#define PACKET_DEBUG_BYTE     'G'  // Add debug packet type

// Add color constants section
/**
 * RGB565 Color Definitions
 * 16-bit color format: RRRR RGGG GGGB BBBB
 */
#define RGB565(r,g,b) ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

// Basic colors
#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_MAGENTA 0xF81F
#define COLOR_CYAN    0x07FF

// Grayscale
#define COLOR_GRAY      0x8410
#define COLOR_DARKGRAY  0x4208
#define COLOR_LIGHTGRAY 0xC618

// Additional colors
#define COLOR_ORANGE  0xFD20
#define COLOR_PURPLE  0x8010
#define COLOR_BROWN   0xA145

// Add state machine constants
/**
 * State Machine Constants
 */
#define STATE_HISTORY_SIZE 16  // Number of states to track in history
#define STATE_NAME_MAX 32      // Maximum length of state name string
#define STATE_TIMEOUT_MS 5000  // Default state timeout
#define STATE_COUNT 8  // Total number of system states

// Add packet validation constants
/**
 * Packet Validation Constants
 */
#define PACKET_MIN_SIZE HEADER_SIZE
#define PACKET_MAX_RETRIES MAX_RETRIES
#define PACKET_SEQUENCE_MAX 255
#define PACKET_VERSION_CURRENT PROTOCOL_VERSION

/**
 * System Timing Constants
 */
#define SYSTEM_TICK_MS          1        // System tick interval
#define SYSTEM_STARTUP_DELAY_MS 2000     // Initial startup delay
#define SYSTEM_WATCHDOG_MS      5000     // Watchdog timeout
#define SYSTEM_YIELD_MS         1        // Yield delay in main loop

/**
 * Recovery System Constants
 */
#define RECOVERY_MAX_ATTEMPTS     8      // Maximum recovery attempts
#define RECOVERY_BASE_DELAY_MS   50      // Initial retry delay
#define RECOVERY_MAX_DELAY_MS   1000     // Maximum retry delay
#define RECOVERY_JITTER_MS       50      // Random jitter range for backoff
#define RECOVERY_STATS_HISTORY   16      // Number of recovery attempts to track

/**
 * State Machine Transitions
 */
// State validation flags
#define STATE_VALID_ENTRY       0x01
#define STATE_VALID_EXIT        0x02
#define STATE_VALID_TRANSITION  0x04
#define STATE_VALID_RESOURCES   0x08
#define STATE_VALID_ALL         0x0F

/**
 * Additional Display Commands
 */
// GC9A01 initialization commands
#define GC9A01_SWRESET          0x01    // Software Reset
#define GC9A01_SLPIN            0x10    // Sleep In
#define GC9A01_SLPOUT           0x11    // Sleep Out
#define GC9A01_INVOFF           0x20    // Display Inversion Off
#define GC9A01_INVON            0x21    // Display Inversion On
#define GC9A01_DISPOFF          0x28    // Display Off
#define GC9A01_DISPON           0x29    // Display On
#define GC9A01_MADCTL           0x36    // Memory Access Control
#define GC9A01_IDMOFF           0x38    // Idle Mode Off
#define GC9A01_IDMON            0x39    // Idle Mode On

// Display memory access control bits
#define MADCTL_MY               0x80    // Row address order
#define MADCTL_MX               0x40    // Column address order
#define MADCTL_MV               0x20    // Row/Column exchange
#define MADCTL_ML               0x10    // Vertical refresh order
#define MADCTL_RGB              0x00    // RGB order
#define MADCTL_BGR              0x08    // BGR order

/**
 * Protocol State Flags
 */
#define PROTOCOL_FLAG_NONE      0x00
#define PROTOCOL_FLAG_INIT      0x01
#define PROTOCOL_FLAG_SYNC      0x02
#define PROTOCOL_FLAG_READY     0x04
#define PROTOCOL_FLAG_TRANSFER  0x08
#define PROTOCOL_FLAG_ERROR     0x10
#define PROTOCOL_FLAG_ALL       0x1F

/**
 * Transfer Status Flags
 */
#define TRANSFER_FLAG_NONE      0x00
#define TRANSFER_FLAG_ACTIVE    0x01
#define TRANSFER_FLAG_COMPLETE  0x02
#define TRANSFER_FLAG_ERROR     0x04
#define TRANSFER_FLAG_TIMEOUT   0x08
#define TRANSFER_FLAG_ALL       0x0F

/**
 * Command Status Flags
 */
#define CMD_FLAG_NONE           0x00
#define CMD_FLAG_ACTIVE         0x01
#define CMD_FLAG_COMPLETE       0x02
#define CMD_FLAG_ERROR          0x04
#define CMD_FLAG_TIMEOUT        0x08
#define CMD_FLAG_ALL            0x0F

/**
 * Protocol Error Codes
 * Specific error codes for protocol-related errors.
 * All codes must be within ERROR_CODE_PROTOCOL_START and ERROR_CODE_PROTOCOL_END range.
 * 
 * Error Code Structure:
 * - 2000-2099: Protocol Version and Initialization
 * - 2100-2199: Packet Structure and Validation
 * - 2200-2299: Data Transfer
 * - 2300-2399: Command Processing
 * - 2900-2999: Fatal Errors
 */

// Protocol Version and Initialization (2000-2099)
#define ERROR_CODE_PROTOCOL_VERSION_MISMATCH (ERROR_CODE_PROTOCOL_START + 1)  // Protocol version mismatch between host and device
                                                                             // Not recoverable - requires firmware update
                                                                             // Context: Expected vs received version

// Packet Structure and Validation (2100-2199)
#define ERROR_CODE_PROTOCOL_SEQUENCE_ERROR   (ERROR_CODE_PROTOCOL_START + 2)  // Packet sequence number out of order
                                                                             // Recoverable - can request packet resend
                                                                             // Context: Expected vs received sequence

#define ERROR_CODE_PROTOCOL_CHECKSUM_ERROR   (ERROR_CODE_PROTOCOL_START + 3)  // Packet checksum validation failed
                                                                             // Recoverable - can request packet resend
                                                                             // Context: Expected vs calculated checksum

#define ERROR_CODE_PROTOCOL_LENGTH_ERROR     (ERROR_CODE_PROTOCOL_START + 4)  // Packet length exceeds maximum or invalid
                                                                             // Recoverable - can request packet resend
                                                                             // Context: Received vs maximum length

// Timing and State (2200-2299)
#define ERROR_CODE_PROTOCOL_TIMEOUT          (ERROR_CODE_PROTOCOL_START + 5)  // Operation timeout occurred
                                                                             // Recoverable - can retry with backoff
                                                                             // Context: Operation and timeout duration

// Fatal Errors (2900-2999)
#define ERROR_CODE_PROTOCOL_FATAL            (ERROR_CODE_PROTOCOL_START + 6)  // Unrecoverable protocol error
                                                                             // Not recoverable - requires system reset
                                                                             // Context: Fatal error description

/**
 * Message Format Constants
 * Two-level message system for logging and errors
 */
#define MESSAGE_PREFIX_LOG    "[LOG]"
#define MESSAGE_PREFIX_ERROR  "[ERROR]"
#define MESSAGE_FORMAT_LOG    "%s [%u] %s: %s%s%s"  // prefix, timestamp, module, message, (context ? " " : ""), (context ? context : "")
#define MESSAGE_FORMAT_ERROR  "%s [%u] %s: %d - %s%s%s"  // prefix, timestamp, module, code, message, (context ? " " : ""), (context ? context : "")

// Message buffer sizes
#define MESSAGE_BUFFER_SIZE   MAX_PACKET_SIZE
#define MESSAGE_MODULE_SIZE   32
#define MESSAGE_TEXT_SIZE     256
#define MESSAGE_CONTEXT_SIZE  128

// NACK Error Flags
#define NACK_ERROR_INVALID_TYPE     0x01
#define NACK_ERROR_VERSION_MISMATCH 0x02
#define NACK_ERROR_CHECKSUM         0x04
#define NACK_ERROR_SEQUENCE         0x08
#define NACK_ERROR_LENGTH           0x10
#define NACK_ERROR_OVERFLOW         0x20
#define NACK_ERROR_TRANSMISSION     0x40

#endif // DESKTHANG_CONSTANTS_H
