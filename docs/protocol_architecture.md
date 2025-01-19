# Protocol Architecture Reference

> Note: All protocol constants referenced in this document are defined in the [Protocol Constants Reference](protocol_constants.md). Always refer to that document for the authoritative values.

## Core Concepts

The protocol implements a robust state machine for managing display communication between a host and device. It handles synchronization, commands, data transfer, and error recovery with clearly defined states and transitions.

## State Machine Architecture

```mermaid
stateDiagram-v2
    [*] --> HARDWARE_INIT: Power On
    
    HARDWARE_INIT --> DISPLAY_INIT: Hardware Ready
    HARDWARE_INIT --> ERROR: Init Failed
    
    DISPLAY_INIT --> IDLE: Display Ready
    DISPLAY_INIT --> ERROR: Init Failed
    
    IDLE --> SYNCING: Sync Request
    IDLE --> ERROR: Invalid Packet
    
    SYNCING --> READY: Sync Valid
    SYNCING --> ERROR: Sync Failed
    SYNCING --> SYNCING: Retry
    
    READY --> COMMAND_PROCESSING: Valid Command
    READY --> DATA_TRANSFER: Image Command
    READY --> ERROR: Invalid Command
    READY --> IDLE: End Command
    
    COMMAND_PROCESSING --> READY: Command Complete
    COMMAND_PROCESSING --> ERROR: Command Failed
    
    DATA_TRANSFER --> DATA_TRANSFER: Chunk Valid
    DATA_TRANSFER --> READY: Transfer Complete
    DATA_TRANSFER --> ERROR: Transfer Failed
    
    ERROR --> IDLE: Reset Complete
    ERROR --> SYNCING: Retry Connection
    
    note right of HARDWARE_INIT
        Initialization Sequence:
        1. Configure SPI/GPIO
        2. Verify timing
        3. Reset controller
        4. Validate hardware state
    end note
    
    note right of DATA_TRANSFER
        Transfer Protocol:
        1. Validate chunk size
        2. Check sequence number
        3. Verify CRC32
        4. Send ACK/NACK
        5. Update display buffer
    end note
    
    note left of ERROR
        Error Recovery:
        1. Log error context
        2. Calculate backoff
        3. Attempt recovery
        4. Reset if needed
    end note
```

## Protocol Layers

### 1. Hardware Interface Layer

Provides hardware abstraction and timing control.

```c
typedef struct {
    // Core configuration
    uint8_t spi_port;      // SPI port number
    uint32_t spi_baud;     // Baud rate (Hz)
    
    // Pin assignments
    struct {
        uint8_t mosi;      // SPI MOSI pin
        uint8_t sck;       // SPI clock pin
        uint8_t cs;        // Chip select pin
        uint8_t dc;        // Data/command pin
        uint8_t rst;       // Reset pin
    } pins;
    
    // Timing parameters
    struct {
        uint32_t reset_pulse_us;    // Reset pulse width
        uint32_t init_delay_ms;     // Post-init delay
        uint32_t cmd_delay_us;      // Command delay
    } timing;
    
    // Status flags
    bool initialized;              // Hardware init complete
    bool display_ready;           // Display init complete
} HardwareConfig;
```

### 2. Serial Communication Layer

Manages USB CDC communication between host and device.

```c
// Core Functions
bool serial_init(uint32_t baud_rate);              // Initialize USB CDC
bool serial_read_exact(uint8_t *buffer,            // Read exact bytes
                      uint16_t size, 
                      uint32_t timeout_ms);
int16_t serial_read(uint8_t *buffer,              // Read available bytes
                   uint16_t size, 
                   uint32_t timeout_ms);
bool serial_write_exact(const uint8_t *buffer,     // Write exact bytes
                       uint16_t size);
int16_t serial_write(const uint8_t *buffer,       // Write available bytes
                    uint16_t size);

// Buffer Management
void serial_flush(void);                          // Flush TX buffer
bool serial_available(void);                      // Check RX buffer
void serial_clear(void);                         // Clear RX buffer

// Key Features
- Protocol-aware buffering (MAX_PACKET_SIZE, CHUNK_SIZE)
- Timeout-based operations with error reporting
- Automatic flush on chunk boundaries
- Integration with error logging system
- Clean connection management
```


### 3. Protocol Layer

Manages packet handling and protocol state.

```c
typedef struct {
    // Protocol version
    uint8_t version;          // Must be 1
    uint8_t sequence;         // Current sequence number
    
    // Timing configuration
    struct {
        uint32_t base_timeout_ms;      // Base timeout (see protocol_constants.md)
        uint32_t min_retry_delay_ms;   // Min retry delay (see protocol_constants.md)
        uint32_t max_retry_delay_ms;   // Max retry delay (see protocol_constants.md)
        uint8_t max_retries;           // Max retry attempts (see protocol_constants.md)
    } timing;
    
    // Buffer configuration
    struct {
        uint16_t max_packet_size;     // Max packet size (see protocol_constants.md)
        uint16_t chunk_size;          // Transfer chunk size (see protocol_constants.md)
        uint8_t header_size;          // Header size in bytes (see protocol_constants.md)
    } limits;
    
    // State tracking
    uint32_t last_checksum;          // Last valid checksum
    uint32_t packets_processed;       // Packet counter
    uint32_t errors_seen;            // Error counter
} ProtocolConfig;
```

### 4. State Management Layer

Controls state transitions and validation.

```c
typedef struct {
    // Current state
    SystemState current;       // Active state
    SystemState previous;      // Previous state
    
    // Timing
    uint32_t entry_time;      // State entry timestamp
    uint32_t duration;        // Time in current state
    
    // Error handling
    uint8_t retry_count;      // Current retry count
    ErrorDetails last_error;  // Last error details
    
    // Validation
    bool transition_valid;    // Last transition valid
    bool in_error_state;     // Error state flag
} StateContext;
```

### 5. Error Management Layer

Handles error detection, logging, and recovery.

The error system is tightly integrated with the state machine:
- All states can transition to ERROR state
- ERROR state has defined recovery paths
- Error context determines recovery strategy
- Recovery success determines next state

### Error and Recovery Integration

The error and recovery systems work together through the following flow:

1. Error Detection
   - System detects error condition
   - ErrorDetails structure is populated
   - Error is logged with ERR! prefix

2. Recovery Selection
   - Recovery strategy chosen based on error type and severity
   - Available strategies: NONE, RETRY, RESET_STATE, REINIT, REBOOT
   - Strategy execution managed by recovery handlers

3. Recovery Execution
   - Handler attempts recovery with configured retries
   - Uses exponential backoff between attempts
   - Reports results through RecoveryResult structure

4. State Transition
   - Success: Move to recovery target state
   - Failure: Remain in ERROR state

Example flow:
```c
// Error detection
error_report(ERROR_TYPE_HARDWARE, ERROR_SEVERITY_ERROR, 1001, "SPI Init Failed");

// Recovery attempt
ErrorDetails *error = error_get_last();
if (error && error_is_recoverable(error)) {
    RecoveryResult result = recovery_attempt(error);
    logging_recovery(&result);
}
```

```c
typedef struct {
    // Error details
    ErrorType type;          // Error type
    ErrorSeverity severity;  // Error severity
    uint32_t code;          // Error code
    uint32_t timestamp;     // When error occurred
    SystemState state;      // State when error occurred
    char message[128];      // Error message
    char context[256];      // Additional context
    bool recoverable;       // Can be recovered from
} ErrorDetails;
```

### Error Code Ranges

Each error type has a dedicated range of error codes:

| Error Type | Code Range | Description |
|------------|------------|-------------|
| HARDWARE   | 1000-1999  | Hardware interface errors (SPI, GPIO, Display) |
| PROTOCOL   | 2000-2999  | Protocol handling errors (packets, sync) |
| STATE      | 3000-3999  | State machine errors (transitions, validation) |
| COMMAND    | 4000-4999  | Command processing errors (validation, execution) |
| TRANSFER   | 5000-5999  | Data transfer errors (chunks, timeouts) |
| SYSTEM     | 6000-6999  | System level errors (memory, resources) |

### Recovery Strategies

1. RECOVERY_NONE
   - Used when error is not recoverable
   - No recovery action taken
   - System remains in error state

2. RECOVERY_RETRY
   - Simple retry of failed operation
   - Uses exponential backoff
   - Limited by MAX_RETRIES

3. RECOVERY_RESET_STATE
   - Resets state machine to known state
   - Clears error condition
   - Maintains hardware configuration

4. RECOVERY_REINIT
   - Full hardware reinitialization
   - Used for hardware failures
   - More aggressive than state reset

5. RECOVERY_REBOOT
   - Complete system restart
   - Last resort recovery
   - Only if allow_reboot is true

### Error Recovery Mapping

| Error Type | Severity | Recovery Strategy | Example |
|------------|----------|------------------|----------|
| HARDWARE   | FATAL    | REBOOT/NONE      | SPI failure |
| HARDWARE   | ERROR    | REINIT           | Display timeout |
| PROTOCOL   | ERROR    | RETRY            | Bad checksum |
| STATE      | ERROR    | RESET_STATE      | Invalid transition |
| COMMAND    | WARNING  | NONE             | Unknown command |
| TRANSFER   | ERROR    | RETRY            | Chunk timeout |
| SYSTEM     | FATAL    | REBOOT           | Memory error |

### Error Logging Format

Error messages follow this format:
```
[LOG] [timestamp] message (ERR!) | Type: type, Severity: severity, Code: code, Recoverable: yes/no
```

Examples:
```
[LOG] [1234] SPI Init Failed (ERR!) | Type: HARDWARE, Severity: FATAL, Code: 1001, Recoverable: No
[LOG] [1235] Invalid Packet (ERR!) | Type: PROTOCOL, Severity: ERROR, Code: 2001, Recoverable: Yes
[LOG] [1236] Invalid State (ERR!) | Type: STATE, Severity: ERROR, Code: 3001, Recoverable: Yes
```

The logging system automatically:
- Adds timestamps
- Formats error details
- Prefixes errors with ERR!
- Includes recovery status

1. Error Detection
   - System detects error condition

## Implementation Requirements

### 1. State Management

- States must be explicitly defined
- All transitions must be validated
- State history must be maintained
- Timing constraints must be enforced
- Error states must be handled

### 2. Error Handling

- Implement exponential backoff
- Log all errors with context
- Support multiple recovery paths
- Track current error state
- Log errors as they occur

### 3. Buffer Management

- Pre-allocate fixed buffers
- Implement boundary checking
- Support partial transfers
- Validate buffer states
- Handle overflow conditions

### 4. Validation Requirements

- Validate all state transitions
- Check packet integrity (CRC32)
- Verify sequence numbers
- Enforce timing constraints
- Validate configuration

## Protocol Timing

### 1. Core Timeouts

See [Protocol Constants Reference](protocol_constants.md#timing-constants) for the authoritative values:
- Base timeout (BASE_TIMEOUT_MS)
- Minimum retry delay (MIN_RETRY_DELAY_MS)
- Maximum retry delay (MAX_RETRY_DELAY_MS)
- Maximum retries (MAX_RETRIES)

### 2. Backoff Algorithm

```c
uint32_t calculate_backoff(uint8_t retry_count) {
    uint32_t delay = MIN_RETRY_DELAY_MS;
    
    // Exponential backoff with jitter
    for (uint8_t i = 0; i < retry_count && delay < MAX_RETRY_DELAY_MS; i++) {
        delay *= 2;
        delay += (rand() % 50);  // Add jitter
    }
    
    return min(delay, MAX_RETRY_DELAY_MS);
}
```

### 3. Retry Strategy

```c
bool recovery_should_retry(uint32_t attempt_count) {
    // Check retry count
    if (attempt_count >= MAX_RETRIES) {
        return false;
    }
    
    return true;
}

uint32_t recovery_get_retry_delay(uint32_t attempt_count) {
    uint32_t delay = MIN_RETRY_DELAY_MS;
    
    // Exponential backoff with jitter
    for (uint32_t i = 0; i < attempt_count && delay < MAX_RETRY_DELAY_MS; i++) {
        delay *= 2;
        delay += (rand() % 50);  // Add jitter
    }
    
    return min(delay, MAX_RETRY_DELAY_MS);
}
```

## Testing Strategy

### 1. State Validation

- Test all valid transitions
- Verify invalid transitions are rejected
- Test error recovery paths
- Validate state history

### 2. Error Recovery

- Test timeout handling
- Verify retry logic
- Test backoff delays
- Validate error logging

### 3. Performance Testing

- Measure round-trip times
- Track retry frequencies
- Monitor buffer usage
- Profile error rates

### 4. Conformance Testing

- Validate packet formats
- Test sequence handling
- Verify checksum calculation
- Test command processing

Strategy selection is based on:
 - Error severity (INFO, WARNING, ERROR, FATAL)
 - Error type (HARDWARE, STATE, etc.)
 - Error context (recoverable flag)
 - System configuration (allow_reboot)

Example log output:
```
[LOG] [1234] Hardware initialization failed (ERR!) | Type: HARDWARE, Severity: FATAL, Code: 1001
```

## Debug Packets

Debug packets contain diagnostic information with the following structure:
- Module name (32 chars max)
- Message (256 chars max)

Example debug packet:
```json
{
"type": "DEBUG",
"module": "Protocol",
"message": "Version mismatch: Expected v1, got v2"
}
```
## Error Handling

Errors are categorized by:
- Type: HARDWARE, STATE, etc.
- Severity: FATAL, WARNING, etc.

Recovery strategies are automatically selected based on error type:
- HARDWARE errors -> Reinitialize hardware
- STATE errors -> Reset state machine
- FATAL errors -> System reboot (if allowed)

Example error packet:
```json


```