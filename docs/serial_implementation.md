# Serial Communication Implementation

## Overview

The serial communication module provides USB CDC (Communications Device Class) functionality for reliable data transfer between the host and device. The implementation wraps the TinyUSB CDC interface while respecting the packet protocol requirements.

## Key Features

- Protocol-aware buffering and transfers
- Timeout-based operations (using BASE_TIMEOUT_MS)
- Automatic flush management
- Simple debug packet transmission
- Clean initialization/deinitialization

## Buffer Management

All buffer sizes are defined in protocol.h:
- Maximum packet size: `MAX_PACKET_SIZE` (512 bytes)
- Chunk size: `CHUNK_SIZE` (256 bytes)
- Static RX buffer for incoming data
- Automatic flush after each chunk write

## Core Functions

### Initialization

```c
bool serial_init(uint32_t baud_rate);
```
- Initializes USB CDC interface
- Waits for USB connection
- Returns true when ready for communication

### Read Operations

```c
bool serial_read_exact(uint8_t *buffer, uint16_t size, uint32_t timeout_ms);
int16_t serial_read(uint8_t *buffer, uint16_t size, uint32_t timeout_ms);
```
- `read_exact`: Blocks until exact number of bytes received or timeout (BASE_TIMEOUT_MS)
- `read`: Returns immediately with available bytes up to requested size
- Both implement timeout-based operation

### Write Operations

```c
bool serial_write_exact(const uint8_t *buffer, uint16_t size);
int16_t serial_write(const uint8_t *buffer, uint16_t size);
```
- `write_exact`: Ensures all bytes are written with automatic flush
- `write`: Best-effort write of available buffer space
- Implements chunked writing with `CHUNK_SIZE` boundaries

### Buffer Control

```c
void serial_flush(void);
bool serial_available(void);
void serial_clear(void);
```
- `flush`: Forces immediate transmission of buffered data
- `available`: Checks for pending received data
- `clear`: Discards any unread received data

## Debug Packet Integration

The serial interface supports debug packet transmission through:
- Simple enabled/disabled control
- Protocol-formatted debug packets (using `MAX_PACKET_SIZE`)
- Module and context information
- Automatic timestamp inclusion
- Non-blocking transmission
- Immediate error logging

## Usage Guidelines

1. Initialize serial communication at system startup
2. Use exact operations for protocol packets
3. Use logging interface for debug messages
4. Check enabled state before formatting debug packets
5. Flush after critical writes
6. Clear buffers when changing protocol states

## Integration Points

- Packet Protocol: Uses `MAX_PACKET_SIZE` and `CHUNK_SIZE` from protocol.h
- Error System: Reports through logging interface
- State Machine: Supports protocol state transitions
- Hardware Layer: Part of hardware abstraction

## Implementation Notes

1. Polling interval of 1ms for non-blocking operations
2. Automatic flush on `CHUNK_SIZE` boundaries
3. Static buffer allocation using `MAX_PACKET_SIZE`
4. Thread-safe volatile buffer pointers
5. Proper USB connection handling

## Error Recovery

- Uses protocol-defined retry counts (`MAX_RETRIES`)
- Implements backoff delays (`MIN_RETRY_DELAY_MS` to `MAX_RETRY_DELAY_MS`)
- Reports errors through logging system
- Supports protocol state recovery
