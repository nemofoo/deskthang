# Protocol Constants Reference

This document serves as the reference for all protocol constants. The authoritative source for these values is `src/protocol/protocol.h`.

## Core Protocol Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `PROTOCOL_VERSION` | 1 | Current protocol version |
| `MAX_PACKET_SIZE` | 512 | Maximum packet size in bytes |
| `CHUNK_SIZE` | 256 | Size of data transfer chunks |
| `HEADER_SIZE` | 8 | Size of packet header in bytes |

## Timing Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `BASE_TIMEOUT_MS` | 1000 | Base timeout in milliseconds |
| `MIN_RETRY_DELAY_MS` | 50 | Minimum retry delay in milliseconds |
| `MAX_RETRY_DELAY_MS` | 1000 | Maximum retry delay in milliseconds |
| `MAX_RETRIES` | 8 | Maximum number of retry attempts |

## Implementation Notes

- All numeric values are defined in `src/protocol/protocol.h`
- These constants are used throughout both the firmware and host implementation
- Any changes to these values must be made in `protocol.h` and will be reflected here
- The host implementation must use identical values for compatibility

## Usage Guidelines

1. **Do not copy these values** to other documentation files
2. **Always reference this document** when documenting protocol specifications
3. **Verify against protocol.h** if any discrepancy is found
4. **Update this document** when protocol.h constants change

This centralized reference helps maintain consistency across the codebase and documentation.
