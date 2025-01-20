# Protocol Documentation

## Overview

The DeskThang protocol implements a packet-based communication system between host and device. It handles command transmission, data transfer, and error detection with a focus on reliable display updates.

## Communication Layer

### USB Serial Interface
- Transport: USB CDC (virtual serial port)
- Buffer Size: 512 bytes
- Flow Control: Handled by pico_stdlib
- Error Detection: Built-in USB error detection

## Packet Structure

### Header Format
```c
typedef struct {
    uint8_t start_marker;     // Always 0xAA
    uint8_t packet_type;      // Command, Data, or Control
    uint16_t sequence;        // Packet sequence number
    uint16_t length;         // Payload length
    uint8_t flags;           // Packet flags
    uint8_t checksum;        // Header checksum
} PacketHeader;
```

### Packet Types
- PACKET_TYPE_COMMAND (0x01): Command packets
- PACKET_TYPE_DATA (0x02): Data transfer packets
- PACKET_TYPE_CONTROL (0x03): Control/status packets

### Flags
- FLAG_START (0x01): Start of transfer
- FLAG_END (0x02): End of transfer
- FLAG_ERROR (0x04): Error condition
- FLAG_ACK_REQ (0x08): Acknowledgment required

## Command Interface

Commands are single ASCII characters that trigger specific actions:

```c
typedef enum {
    CMD_IMAGE_START = 'I',    // Start image transfer (RGB565 format, 240×240)
    CMD_IMAGE_DATA = 'D',     // Image data chunk
    CMD_IMAGE_END = 'E',      // End image transfer
    CMD_PATTERN_CHECKER = '1', // Show checkerboard pattern
    CMD_PATTERN_STRIPE = '2',  // Show stripe pattern
    CMD_PATTERN_GRADIENT = '3',// Show gradient pattern
    CMD_HELP = 'H'            // Display help/command list
} CommandType;
```

### Command Context
```c
typedef struct {
    CommandType type;          // Active command
    uint32_t start_time;       // Command start timestamp
    uint32_t bytes_processed;  // Progress tracking
    uint32_t total_bytes;      // Total expected bytes
    size_t data_size;         // Current chunk size
    bool in_progress;          // Command active flag
    void *command_data;        // Command-specific data
} CommandContext;
```

### Command Status
```c
typedef struct {
    bool success;              // Command success/failure
    uint32_t duration_ms;      // Execution time
    uint32_t bytes_processed;  // Data processed
    char message[256];         // Status message
} CommandStatus;
```

## Data Transfer

### Image Transfer Protocol
1. Start Transfer
   ```
   [Header][CMD_IMAGE_START]
   - Flags: FLAG_START
   - Length: 0
   ```

2. Data Chunks
   ```
   [Header][RGB565 Data]
   - Flags: 0
   - Length: Up to 512 bytes
   ```

3. End Transfer
   ```
   [Header][CMD_IMAGE_END]
   - Flags: FLAG_END
   - Length: 0
   ```

### Transfer Validation
- Sequence number tracking
- Packet checksums
- Size validation
- Format verification

## Error Handling

### Protocol Errors
1. Packet Errors
   - Invalid start marker
   - Checksum mismatch
   - Invalid sequence
   - Length mismatch

2. Command Errors
   - Invalid command
   - Invalid state
   - Resource unavailable

3. Transfer Errors
   - Buffer overflow
   - Incomplete transfer
   - Format error
   - Timeout

### Error Recovery
1. Packet Recovery
   - Retransmission requests
   - Sequence resynchronization
   - Buffer cleanup

2. Command Recovery
   - Command abort
   - State reset
   - Error acknowledgment

## Example Usage

### Command Protocol

All commands must be sent using the packet protocol, which provides error checking and reliable delivery. Each command is wrapped in a packet with a header:

```bash
# Show test patterns
send_packet CMD_PATTERN_CHECKER > /dev/ttyACM0  # Checkerboard (command '1')
send_packet CMD_PATTERN_STRIPE > /dev/ttyACM0   # Color bars (command '2')
send_packet CMD_PATTERN_GRADIENT > /dev/ttyACM0 # Gradient (command '3')

# Show help
send_packet CMD_HELP > /dev/ttyACM0  # Help (command 'H')

# Image transfer
# Start transfer
send_packet CMD_IMAGE_START > /dev/ttyACM0

# Send image data in packets
send_image_packets image.raw > /dev/ttyACM0

# End transfer
send_packet CMD_IMAGE_END > /dev/ttyACM0
```

The packet protocol ensures reliable command delivery with:
- Sequence tracking
- Error detection
- Acknowledgments
- Retransmission on failure

The packet format for complex commands is:
```c
PacketHeader:
  - start_marker (0xAA)
  - packet_type  (CMD/DATA/CONTROL)
  - sequence     (0-255)
  - length       (payload size)
  - checksum     (CRC32)
Payload:
  - Command data
```

## Performance Considerations

1. Transfer Optimization
   - 512-byte packet size
   - Minimal overhead
   - Efficient buffering

2. Command Processing
   - Immediate execution
   - Minimal latency
   - Efficient validation

3. Error Recovery
   - Fast detection
   - Quick recovery
   - Minimal retransmission
