# DeskThang Display Project

A desk display project using Raspberry Pi Pico (RP2040) with a GC9A01 LCD screen. The project consists of a Pico firmware and a Zig host application that can push images and patterns to the display over USB.

## Setup

Clone the repository with submodules:

```bash
git clone [repository-url]
git submodule update --init --recursive
```

Build the Pico firmware:

```bash
cd pico
mkdir build
cd build
cmake ..
make
```

Build the Zig host application:

```bash
cd host
zig build
```

## Protocol Overview

The project uses a state machine-based protocol for reliable communication between host and device. See the protocol documentation for complete details:

- [Protocol Architecture](docs/protocol_architecture.md)
- [State Machine](docs/protocol_state_machine.md)
- [State Transitions](docs/protocol_transitions.md)
- [Protocol Modules](docs/protocol_modules.md)

### Core Features

- Version: 1 (sole supported version)
- State machine architecture
- Robust error handling and recovery
- Modular implementation
- Comprehensive validation

### Packet Structure

- Header (8 bytes):
  - Packet Type (1 byte)
  - Sequence Number (1 byte)
  - Length (2 bytes)
  - CRC32 Checksum (4 bytes)
- Payload (variable length, max 512 bytes)

### Packet Types

- SYNC (0x1B): Synchronization request
- SYNC_ACK (0x1C): Synchronization acknowledgment
- CMD (0x1D): Command packet
- DATA (0x1E): Data packet
- ACK (0x1F): Acknowledgment
- NACK (0x20): Negative acknowledgment

### Commands

- I: Start image transfer (RGB565 format, 240×240)
- E: End image transfer
- 1: Show checkerboard pattern
- 2: Show stripe pattern
- 3: Show gradient pattern
- H: Display help/command list

### Protocol States

1. HARDWARE_INIT: Initial hardware setup
2. DISPLAY_INIT: Display controller initialization
3. IDLE: Default waiting state
4. SYNCING: Protocol synchronization
5. READY: Connection established
6. COMMAND_PROCESSING: Handling commands
7. DATA_TRANSFER: Image transfer
8. ERROR: Error handling and recovery

### Error Handling

The protocol implements comprehensive error handling:

- State validation
- Packet integrity checks
- Sequence validation
- Timeout management
- Automatic retries with exponential backoff
- Error logging and recovery

Key parameters:
- Maximum 8 retries per operation
- Base timeout: 1000ms
- Retry delays: 50ms to 1000ms (with exponential backoff)
- Data chunks: 256 bytes

## Module Organization

### Firmware (C)

```
src/
├── hardware/           # Hardware abstraction layer
│   ├── hardware.c     # Core hardware interface
│   ├── spi.c         # SPI implementation
│   ├── gpio.c        # GPIO implementation
│   └── display.c     # Display driver
│
├── protocol/          # Protocol implementation
│   ├── protocol.c    # Core protocol handler
│   ├── packet.c      # Packet management
│   ├── command.c     # Command processing
│   └── transfer.c    # Data transfer
│
├── state/            # State management
│   ├── state.c       # State machine core
│   ├── context.c     # State context
│   └── transition.c  # State transitions
│
└── error/            # Error handling
    ├── error.c       # Error management
    ├── recovery.c    # Recovery strategies
    └── logging.c     # Error logging
```

### Host Application (Zig)

```
host/
├── build.zig         # Zig build configuration
├── build.zig.zon     # Zig dependency manifest
│
└── src/              # Source code
    ├── main.zig      # Application entry point
    ├── protocol.zig  # Protocol implementation
    ├── picocom.zig   # Serial communication
    └── root.zig      # Root namespace
```

## Hardware Setup

- Screen: GC9A01 240×240 Round LCD
- Microcontroller: Raspberry Pi Pico (RP2040)
- Connections:
  - MOSI: GPIO 19
  - SCK: GPIO 18
  - CS: GPIO 17
  - DC: GPIO 16
  - RST: GPIO 20

## Dependencies

- Pico SDK (submodule)
- Zig 0.11.0 or later
- CMake 3.13 or later
- libpng for the host application

## Serial Communication

- Baud rate: 115200
- Flow control: None
- Protocol channel: Clean protocol data only
- Debug channel: Optional separate channel
  - If single channel used: Prefix debug with "DBG:"
  - Never mix debug and protocol data

## Implementation Requirements

### Host Implementation
- Must implement full state machine
- Must handle all error conditions
- Must support retry mechanisms
- Must validate all transitions

### Device Implementation
- Must maintain consistent state
- Must respond to all commands
- Must implement error recovery
- Must validate all packets

## Debug Support

- State transition logging
- Error context tracking
- Performance monitoring
- Resource usage tracking
- Timing statistics

## Testing Requirements

1. State Transitions
- Test all valid transitions
- Verify invalid transitions are rejected
- Test boundary conditions
- Validate state history

2. Error Handling
- Test all error conditions
- Verify recovery paths
- Test retry mechanisms
- Validate error logging

3. Resource Management
- Test buffer allocation
- Verify cleanup on exit
- Test resource limits
- Validate memory usage

4. Performance
- Measure transition times
- Track error rates
- Monitor resource usage
- Profile critical paths

For detailed protocol implementation information, refer to the documentation in the docs/ directory.
