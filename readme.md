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

## Communication Protocol (v1)

All host-device communication occurs over USB serial using a packet-based protocol:

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

### Protocol Features

- Version: 1 (the only supported protocol version)
- CRC32 error detection
- Sequence numbering for packet ordering
- Automatic retransmission on errors
- Exponential backoff retry mechanism
- Maximum 8 retries per packet
- Base timeout of 1000ms
- Retry delays between 50ms and 1000ms
- Data chunks of 256 bytes

### Commands

- I: Start image transfer (RGB565 format, 240×240)
- E: End image transfer
- 1: Show checkerboard pattern
- 2: Show stripe pattern
- 3: Show gradient pattern
- H: Display help/command list

### Error Handling

The protocol handles:

- Invalid synchronization
- Checksum mismatches
- Sequence errors
- Timeouts
- Negative acknowledgments
- Invalid packet types

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

## Serial Communication Setup

- Baud rate: 115200
- Flow control: None
- Debug output: Separate channel, or use a prefix like "DBG:" on the same channel that the host filters out from protocol data.

## Protocol Framing

- The first packet from the host begins with SYNC (0x1B).
- All subsequent packet headers align on 8-byte boundaries.
- The device must not send non-protocol data on the protocol channel.
- If debug output is needed during protocol operation and there is only a single interface:
  - Prefix logs with "DBG:" so the host ignores them for protocol parsing.
  - Never mix debug and protocol bytes in the same packet.
