# Protocol Implementation Discrepancies

This document outlines discrepancies between the documented protocol specification and the current C (device) and Zig (host) implementations.

## State Machine States

### Specification
States:
- IDLE
- SYNCING
- READY
- SENDING_COMMAND
- RECEIVING_DATA
- ERROR

### Actual Implementations
- **Zig (Host)**
  ```zig
  pub const State = enum(u8) { 
    idle,
    syncing,
    ready, 
    sending_command, 
    receiving_data, 
    error_state 
  };
  ```

- **C (Device)**
  - Implements state machine implicitly through function flow
  - Missing explicit ERROR state handling

**Impact**: The C implementation lacks explicit state tracking and proper error state handling as required by the spec.

## Error Handling

### Specification
Defines specific error recovery paths:
- Sync Loss Recovery
- Checksum Error Recovery
- Sequence Error Recovery
- Timeout Recovery

### Actual Implementations
- **Zig (Host)**
  ```zig
  pub const Error = error{
      InvalidSync,
      InvalidChecksum,
      InvalidSequence,
      Timeout,
      NackReceived,
      InvalidPacketType,
  };
  ```

- **C (Device)**
  - Basic error handling through NACK responses
  - Missing explicit error recovery strategies
  - No exponential backoff implementation
  - Incomplete timeout handling

**Impact**: Neither implementation fully implements the specified error recovery strategies.

## Timing Parameters

### Specification
- Base Timeout: 1000ms
- Min Retry Delay: 50ms
- Max Retry Delay: 1000ms
- Max Retries: 8

### Actual Implementations
- **Zig (Host)**
  ```zig
  pub const MAX_RETRIES: u8 = 8;
  pub const BASE_TIMEOUT_MS: u64 = 1000;
  pub const MIN_RETRY_DELAY_MS: u64 = 50;
  pub const MAX_RETRY_DELAY_MS: u64 = 1000;
  ```

- **C (Device)**
  ```c
  #define SERIAL_TIMEOUT_US 2000000    // 2 seconds base timeout
  #define SERIAL_DELAY_MS 20           // Different from spec
  ```

**Impact**: C implementation uses different timing parameters than specified.

## Packet Structure

### Specification
- Maximum Packet Size: 512 bytes
- Header Size: 8 bytes
- Chunk Size: 256 bytes

### Actual Implementations
Both implementations match these parameters, but with different approaches:

- **Zig (Host)**
  ```zig
  pub const CHUNK_SIZE: usize = 256;
  pub const MAX_PACKET_SIZE: usize = 512;
  pub const HEADER_SIZE: usize = 8;
  ```

- **C (Device)**
  ```c
  #define CHUNK_SIZE 256
  #define BUFFER_SIZE 512
  // Header size defined through struct
  ```

**Conformance**: This is one area where both implementations closely match the spec.

## Future Work Tasks

1. **State Machine Alignment**
   - Add explicit state tracking to C implementation
   - Implement proper ERROR state handling in C code

2. **Error Recovery**
   - Implement full error recovery strategies in both implementations
   - Add exponential backoff for retries
   - Add proper error logging and tracking
   - Implement complete timeout handling

3. **Timing Standardization**
   - Update C implementation to use specified timing parameters
   - Add timeout validation
   - Implement consistent retry delays

4. **Protocol Validation**
   - Add conformance testing suite
   - Implement state validation matrix checks
   - Add packet structure validation

5. **Documentation**
   - Update spec to clarify any intentional deviations
   - Add implementation-specific notes
   - Document error handling requirements

## Priority Order

1. Implement proper error handling (High Priority)
   - Critical for robust operation
   - Currently incomplete in both implementations

2. Standardize timing parameters (Medium Priority)
   - Important for reliable communication
   - Currently inconsistent between implementations

3. Add state machine validation (Medium Priority)
   - Helps ensure protocol correctness
   - Makes debugging easier

4. Improve documentation (Lower Priority)
   - Needed for long-term maintenance
   - Should reflect final implementation decisions
