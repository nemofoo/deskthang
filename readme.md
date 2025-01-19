# DeskThang Display Project

A desk display project using Raspberry Pi Pico (RP2040) with a GC9A01 LCD screen. The project consists of a Pico firmware and a Zig host application that can push images and patterns to the display over USB.

## Setup

1. Clone the repository with submodules:
```bash
git clone [repository-url]
git submodule update --init --recursive
```

2. Build the Pico firmware:
```bash
cd pico
mkdir build
cd build
cmake ..
make
```

3. Build the Zig host application:
```bash
cd host
zig build
```

## communication protocol (v1)

The host and Pico communicate over USB serial using a robust packet-based protocol:

### Packet Structure
- Header (8 bytes):
  - Packet Type (1 byte)
  - Sequence Number (1 byte)
  - Length (2 bytes)
  - CRC32 Checksum (4 bytes)
- Payload (variable length, max 512 bytes)

### Packet Types
- `SYNC (0x1B)`: Synchronization request
- `SYNC_ACK (0x1C)`: Synchronization acknowledgment
- `CMD (0x1D)`: Command packet
- `DATA (0x1E)`: Data packet
- `ACK (0x1F)`: Acknowledgment
- `NACK (0x20)`: Negative acknowledgment

### Protocol Features
- CRC32 error detection
- Sequence numbering for packet ordering
- Automatic retransmission on errors
- Exponential backoff retry mechanism
- Maximum 8 retries per packet
- Base timeout of 1000ms
- Retry delays between 50ms and 1000ms
- Data chunks of 256 bytes

### Commands
- `I`: Start image transfer (RGB565 format, 240x240)
- `E`: End image transfer
- `1`: Show checkerboard pattern
- `2`: Show stripe pattern
- `3`: Show gradient pattern
- `H`: Display help/command list

### Error Handling
The protocol handles various error conditions:
- Invalid synchronization
- Checksum mismatches
- Sequence errors
- Timeouts
- Negative acknowledgments
- Invalid packet types

## Hardware Setup
- Screen: GC9A01 240x240 Round LCD
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
