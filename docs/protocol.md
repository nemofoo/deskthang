# DeskThang Protocol Documentation

## Overview
The DeskThang protocol is a reliable communication system that wraps all messages in packets with consistent framing and validation.

## Packet Structure
Each packet consists of three parts:

1. Start Metadata (Header)
   - Start marker (`~`)
   - Type identifier (DEBUG, COMMAND, DATA, etc.)
   - Sequence number
   - Payload length

2. Payload
   - Variable length data
   - Content depends on packet type

3. End Metadata
   - Space character
   - 8-character hexadecimal CRC32 checksum (e.g., "A5B2C3D4")
   - End marker (`\n`)

## Example Packets
```
~  SYSTEM: Heartbeat: 0, State: IDLE 1234ABCD\n
~  ERROR: Buffer overflow in display driver 5678DEEF\n
```

## Packet Types
- DEBUG: System debug messages and logs
- COMMAND: Control commands
- DATA: Raw image chunk data
- ACK: Command acknowledgments
- NACK: Command rejection/errors
- SYNC: Protocol synchronization
- ERROR: System/hardware error reports

## Special Characters
- `~`: Start marker
- `\n`: End marker
- `\`: Escape character (used to escape special characters in payload)

## Implementation Notes
1. All special characters in the payload must be escaped using `\` followed by the character XORed with 0x20
2. The checksum is calculated on the raw (unescaped) data
3. The checksum is displayed in uppercase hexadecimal with leading zeros
4. A space character separates the payload from the checksum

## Monitoring
To monitor the protocol:
```bash
# Configure serial port
stty -F /dev/ttyACM0 115200 raw -echo -echoe -echok

# Monitor traffic
cat /dev/ttyACM0
```
