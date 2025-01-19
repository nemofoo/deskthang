# Protocol Architecture Reference

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

### 2. Protocol Layer

Manages packet handling and protocol state.

```c
typedef struct {
    // Protocol version
    uint8_t version;          // Must be 1
    uint8_t sequence;         // Current sequence number
    
    // Timing configuration
    struct {
        uint32_t base_timeout_ms;      // Base timeout (1000ms)
        uint32_t min_retry_delay_ms;   // Min retry delay (50ms)
        uint32_t max_retry_delay_ms;   // Max retry delay (1000ms)
        uint8_t max_retries;           // Max retry attempts (8)
    } timing;
    
    // Buffer configuration
    struct {
        uint16_t max_packet_size;     // Max packet size (512)
        uint16_t chunk_size;          // Transfer chunk size (256)
        uint8_t header_size;          // Header size in bytes (8)
    } limits;
    
    // State tracking
    uint32_t last_checksum;          // Last valid checksum
    uint32_t packets_processed;       // Packet counter
    uint32_t errors_seen;            // Error counter
} ProtocolConfig;
```

### 3. State Management Layer

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
    ErrorContext last_error;  // Last error details
    
    // Validation
    bool transition_valid;    // Last transition valid
    bool in_error_state;     // Error state flag
} StateContext;
```

### 4. Error Management Layer

Handles error detection, logging, and recovery.

```c
typedef struct {
    // Error details
    ErrorType type;              // Error classification
    SystemState source_state;    // State where error occurred
    uint32_t timestamp;         // Error timestamp
    
    // Context data
    uint32_t error_code;        // Specific error code
    char message[64];           // Error description
    
    // Recovery
    uint8_t retry_count;        // Recovery attempts
    uint32_t backoff_ms;        // Current backoff delay
    bool recoverable;           // Can be recovered from
} ErrorContext;
```

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
- Track error statistics
- Maintain error history

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

- Base timeout: 1000ms
- Minimum retry delay: 50ms
- Maximum retry delay: 1000ms
- Maximum retries: 8

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
bool should_retry(ErrorContext *ctx) {
    // Check retry count
    if (ctx->retry_count >= MAX_RETRIES) {
        return false;
    }
    
    // Calculate backoff
    uint32_t delay = calculate_backoff(ctx->retry_count);
    ctx->backoff_ms = delay;
    
    // Update context
    ctx->retry_count++;
    
    return true;
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
