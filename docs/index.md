# DeskThang Documentation

## Overview

DeskThang is a USB-connected display device featuring a GC9A01 240×240 round LCD. It provides a reliable interface for displaying images and test patterns through a robust packet-based protocol.

## Hardware

- **Display**: GC9A01 240×240 Round LCD ([display details](display.md))
  - RGB565 color format (16-bit color)
  - SPI interface (Mode 0)
  - Round display shape

## Command Interface

DeskThang accepts the following commands, each wrapped in the packet protocol for reliable delivery:

1. **Test Patterns**
   - `CMD_PATTERN_CHECKER ('1')`: Checkerboard pattern
   - `CMD_PATTERN_STRIPE ('2')`: Color bars pattern
   - `CMD_PATTERN_GRADIENT ('3')`: RGB gradient pattern

2. **Image Transfer**
   - `CMD_IMAGE_START ('I')`: Begin image transfer
   - `CMD_IMAGE_DATA ('D')`: Send image data chunks
   - `CMD_IMAGE_END ('E')`: Complete image transfer

3. **System**
   - `CMD_HELP ('H')`: Display command help

For detailed protocol information, see the [protocol documentation](protocol.md).

## Packet Protocol

All commands are wrapped in packets to ensure reliable delivery. Here's how the protocol works:

```mermaid
graph TD
    subgraph "Host Computer"
        CMD[Command<br>'1' = Checkerboard]
        RETRY[Retry Queue]
    end
    
    subgraph "Protocol Layer"
        PKT[Create Packet]
        SEQ[Add Sequence<br>0-255]
        CRC[Add CRC32<br>Checksum]
        SEND[Send via USB]
    end
    
    subgraph "DeskThang Device"
        RCV[Receive Packet]
        CHK[Check Header<br>0xAA Marker]
        VAL[Validate<br>CRC + Sequence]
        PROC[Process Command]
        DISP[Update Display]
    end
    
    subgraph "Response"
        ACK[Send ACK]
        NACK[Send NACK]
    end
    
    CMD --> PKT
    PKT --> SEQ
    SEQ --> CRC
    CRC --> SEND
    
    SEND --> RCV
    RCV --> CHK
    CHK -->|Valid| VAL
    CHK -->|Invalid| NACK
    
    VAL -->|Valid| PROC
    VAL -->|Invalid| NACK
    
    PROC --> DISP
    PROC --> ACK
    
    NACK --> RETRY
    RETRY --> PKT
    
    style DISP fill:#98FB98
    style NACK fill:#FFB6C1
    style ACK fill:#98FB98
```

The protocol ensures reliable command delivery through:
1. **Packet Creation**
   - Wraps command in header
   - Adds sequence number
   - Calculates CRC32 checksum

2. **Transmission**
   - Sends via USB CDC interface
   - Monitors for acknowledgment
   - Queues for retry if needed

3. **Reception**
   - Validates packet marker
   - Verifies sequence number
   - Checks CRC32 integrity

4. **Processing**
   - Executes valid commands
   - Updates display if needed
   - Sends acknowledgment

5. **Error Handling**
   - Detects corrupted packets
   - Requests retransmission
   - Maintains sequence order

### Why Packets?

The packet protocol provides:
1. **Reliability**: Sequence numbers detect lost or duplicate commands
2. **Integrity**: Checksums ensure data isn't corrupted
3. **Recovery**: Automatic retransmission of failed commands
4. **Validation**: Command parameters are verified before execution

For details on state management, see the [state machine documentation](state_machine.md).

## Example Usage

```bash
# Send a command to display checkerboard pattern
send_packet CMD_PATTERN_CHECKER > /dev/ttyACM0

# Transfer an image
send_packet CMD_IMAGE_START > /dev/ttyACM0
send_image_packets image.raw > /dev/ttyACM0
send_packet CMD_IMAGE_END > /dev/ttyACM0
```

Each command is:
1. Wrapped in a packet header
2. Assigned a sequence number
3. Protected with a checksum
4. Acknowledged by the device
5. Retransmitted if errors occur

## Packet Structure

```mermaid
graph LR
    subgraph "Packet Format"
        H1[Start<br>0xAA] --> H2[Type<br>CMD/DATA]
        H2 --> H3[Sequence<br>0-255]
        H3 --> H4[Length<br>N bytes]
        H4 --> H5[Flags]
        H5 --> H6[Checksum]
        H6 --> P1[Payload<br>Command/Data]
    end
```

The packet structure ensures:
- **Synchronization**: Start marker identifies packet boundaries
- **Ordering**: Sequence numbers maintain command order
- **Size Control**: Length field prevents buffer overflows
- **Error Detection**: Checksum validates data integrity

For complete protocol details, see the [protocol documentation](protocol.md).

## State Machine

The device uses a state machine to manage its operation and ensure safe command processing:

```mermaid
stateDiagram-v2
    [*] --> HARDWARE_INIT
    
    subgraph "Initialization"
        HARDWARE_INIT --> DISPLAY_INIT: Hardware Ready
        DISPLAY_INIT --> IDLE: Display Ready
    end
    
    subgraph "Command Processing"
        IDLE --> COMMAND_PROCESSING: Packet Received
        COMMAND_PROCESSING --> DATA_TRANSFER: Image Start
        DATA_TRANSFER --> COMMAND_PROCESSING: Transfer Complete
        COMMAND_PROCESSING --> IDLE: Command Complete
    end
    
    subgraph "Error Handling"
        HARDWARE_INIT --> ERROR: Init Failed
        DISPLAY_INIT --> ERROR: Init Failed
        COMMAND_PROCESSING --> ERROR: Command Failed
        DATA_TRANSFER --> ERROR: Transfer Failed
        ERROR --> IDLE: Recovery Complete
    end
```

### State Flow
1. **Initialization**
   - Device starts in HARDWARE_INIT
   - Configures SPI, GPIO, clocks
   - Initializes display hardware
   - Transitions to IDLE when ready

2. **Command Processing**
   - IDLE state waits for packets
   - Validates each packet
   - Processes commands in order
   - Handles data transfers

3. **Error Recovery**
   - Detects failures at each stage
   - Attempts automatic recovery
   - Returns to IDLE when fixed
   - Logs error information

For complete state machine details, see the [state machine documentation](state_machine.md).

## Image Transfer

Transferring images requires multiple packets due to the display's size (240×240 RGB565 = 115,200 bytes). Here's how it works:

```mermaid
sequenceDiagram
    participant H as Host
    participant D as Device
    participant B as Buffer
    participant LCD as Display
    
    Note over H,LCD: Start Transfer
    H->>D: CMD_IMAGE_START Packet
    D->>B: Initialize Buffer
    D-->>H: ACK
    
    Note over H,LCD: Data Transfer
    loop Until Complete
        H->>D: CMD_IMAGE_DATA Packet (512 bytes)
        D->>B: Add to Buffer
        D-->>H: ACK
    end
    
    Note over H,LCD: End Transfer
    H->>D: CMD_IMAGE_END Packet
    D->>B: Validate Complete Image
    D->>LCD: Write to Display
    D-->>H: ACK
    
    Note over H,LCD: Error Handling
    alt Data Error
        D-->>H: NACK
        H->>D: Retransmit Packet
    end
```

### Transfer Process
1. **Start Transfer**
   - Host sends CMD_IMAGE_START
   - Device allocates buffer
   - Size: 240×240×2 bytes (RGB565)

2. **Data Transfer**
   - Data sent in 512-byte chunks
   - Each chunk wrapped in packet
   - Sequence numbers ensure order

3. **End Transfer**
   - Host sends CMD_IMAGE_END
   - Device validates complete image
   - Writes to display if valid

4. **Error Recovery**
   - Checksums verify each chunk
   - Failed chunks retransmitted
   - Buffer cleaned up on error

For details on the transfer protocol, see the [protocol documentation](protocol.md).

## Test Patterns

The device includes built-in test patterns to verify display operation:

```mermaid
graph TD
    subgraph "Test Pattern Commands"
        direction LR
        CMD1["CMD_PATTERN_CHECKER ('1')"] --> P1[Checkerboard]
        CMD2["CMD_PATTERN_STRIPE ('2')"] --> P2[Color Bars]
        CMD3["CMD_PATTERN_GRADIENT ('3')"] --> P3[RGB Gradient]
    end
    
    subgraph "Pattern Details"
        P1 --> CB[20×20 pixel squares<br>Black and White]
        P2 --> ST[8 vertical bars<br>R,G,B,Y,M,C,W,K]
        P3 --> GR[Smooth RGB blend<br>X=Red, Y=Green]
    end
```

### Pattern Descriptions

1. **Checkerboard Pattern**
   - 20×20 pixel squares
   - Alternating black and white
   - Tests contrast and alignment
   - Verifies pixel boundaries

2. **Color Bars Pattern**
   - 8 vertical color bars
   - Basic colors: Red, Green, Blue
   - Mixed colors: Yellow, Magenta, Cyan
   - Endpoints: White, Black
   - Tests color reproduction

3. **RGB Gradient Pattern**
   - Smooth color transitions
   - Red varies with X position
   - Green varies with Y position
   - Blue varies diagonally
   - Tests color interpolation

Each pattern is useful for:
- Display initialization testing
- Color calibration verification
- Visual quality assessment
- Hardware validation

For details on display capabilities, see the [display documentation](display.md).
