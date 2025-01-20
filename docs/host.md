# Host Application (Zig)

## Overview

The host application is responsible for communicating with the DeskThang device over USB serial, implementing the packet protocol and state machine client. It provides a command-line interface for sending test patterns and images to the display.

## Core Requirements

1. **Protocol Implementation**
   - Packet creation and validation
   - Sequence number management
   - CRC32 checksum calculation
   - Packet retry queue with exponential backoff
   - Clean separation of protocol and debug channels
   - Full communication logging
     - All sent packets
     - All received packets
     - Debug messages from device
     - Timestamps for each event

2. **State Machine Client**
   - Track device state
   - Validate state transitions
   - Handle state-specific commands
   - Error recovery procedures

3. **Command Interface**
   - Test pattern commands
     - Checkerboard (`'1'`)
     - Color bars (`'2'`)
     - Gradient (`'3'`)
   - Image transfer
     - Start transfer (`'I'`)
     - Send data chunks (`'D'`)
     - End transfer (`'E'`)
   - Help/status (`'H'`)

## Implementation Tasks

### Phase 1: Core Protocol ✅
- [x] Implement packet structure
  - Header with type, sequence, length, CRC32
  - Payload handling
  - Packet validation
- [x] Add serial communication
  - USB CDC device detection
  - Read/write with timeout
  - Error handling
- [x] Create packet types
  - SYNC (0x1B)
  - SYNC_ACK (0x1C)
  - CMD (0x1D)
  - DATA (0x1E)
  - ACK (0x1F)
  - NACK (0x20)
- [⚠️] Implement logging
  - Log file creation ✅
  - Packet logging (sent/received) ✅
  - Debug message capture ✅
  - Timestamp formatting needs improvement
  - Human-readable packet dumps ✅

### Phase 2: State Machine ✅
- [x] Implement state tracking
  - Current state validation
  - Transition validation
  - Error state handling
- [x] Add retry mechanism
  - Exponential backoff
  - Maximum retry limit
- [x] Create state handlers
  - IDLE
  - SYNCING
  - READY
  - SENDING_COMMAND
  - RECEIVING_DATA
  - ERROR_STATE

### Phase 3: Commands ⚠️
- [x] Test pattern commands
  - Command validation
  - State checks
  - Response handling
- [⚠️] Image transfer
  - PNG validation ✅
    - Must be PNG format
    - Must be exactly 240×240 pixels
    - Early validation before any transfer
  - RGB565 format conversion ✅
  - Chunked transfer ⚠️
  - Progress tracking ⚠️

## Project Structure

```
host/
├── src/
│   ├── main.zig          # Entry point and CLI
│   ├── protocol/
│   │   ├── packet.zig    # Packet handling and CRC32
│   │   ├── serial.zig    # Serial communication
│   │   ├── transfer.zig  # Data transfer handling
│   │   ├── state.zig     # State machine
│   │   ├── constants.zig # Protocol constants
│   │   └── logger.zig    # Communication logger
│   └── command/
│       └── image.zig     # Image transfer and conversion
├── build.zig            # Build configuration
└── .gitignore          # Git ignore rules
```

## Dependencies

- `std.io`: Serial port handling
- `std.crypto`: CRC32 calculation
- `std.Thread`: Async operations
- `libpng`: PNG image loading and validation

## Error Handling

1. **Communication Errors**
   - Device not found
   - Serial port errors
   - Timeout handling
   - Retry management

2. **Protocol Errors**
   - Invalid packets
   - CRC mismatches
   - Sequence errors
   - State violations

3. **User Input Errors**
   - Invalid commands
   - File not found
   - Format errors
   - Permission issues
   - Image validation errors:
     - Not a PNG file
     - Wrong dimensions (must be 240×240)
     - Unsupported color format

## Testing Strategy

1. **Unit Tests** (To Be Implemented)
   - Packet creation/validation
   - CRC calculation
   - State transitions
   - Command formatting

2. **Integration Tests** (To Be Implemented)
   - Protocol sequence
   - State machine flow
   - Error recovery
   - Full transfer cycle

3. **Manual Tests** (To Be Implemented)
   - Pattern display
   - Image transfer
   - Error conditions
   - User interface

## Usage Examples

```bash
# Basic commands (logs are automatically written to serial.log)
deskthang test 1    # Show checkerboard pattern
deskthang test 2    # Show color bars pattern
deskthang test 3    # Show gradient pattern
deskthang image image.png  # Send 240×240 PNG image
```

## Log Format

Current log format:
```
[1705789532] TX: SYNC seq=1 len=0 crc=0x12345678
[1705789532] RX: SYNC_ACK seq=1 len=0 crc=0x87654321
[1705789532] DEBUG: "Display initialized successfully"
[1705789532] TX: CMD seq=2 len=1 crc=0x11223344 payload="1"
[1705789532] RX: ACK seq=2 len=0 crc=0x44332211
```

Planned improvements:
- Add human-readable timestamps with millisecond precision
- Add packet type hex codes in log output
- Improve debug message formatting

## Development Guidelines

1. **Error Handling**
   - All errors must be properly handled
   - No unwrapped errors in production
   - Clear error messages with suggestions
     - For image errors, specify exact requirements
     - Suggest image resizing tools if wrong dimensions
   - Recovery instructions

2. **State Management**
   - Validate all transitions
   - Handle timeout/retry
   - Clean state recovery
   - Debug logging

3. **Code Style**
   - Follow Zig style guide
   - Clear documentation
   - Error union types
   - Proper allocator usage 