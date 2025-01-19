# Protocol State Machine Reference

> Note: All protocol constants referenced in this document (timeouts, retry counts, etc.) are defined in the [Protocol Constants Reference](protocol_constants.md). Always refer to that document for the authoritative values.

## Overview

The protocol implements a deterministic state machine that manages all aspects of display communication. This document defines the reference implementation that both host and device must follow.

## Core Requirements

### Host Implementation
- Must implement full state machine
- Must handle all error conditions
- Must support retry mechanisms (see protocol_constants.md for retry limits and timing)
- Must validate all transitions

### Device Implementation  
- Must maintain consistent state
- Must respond to all commands
- Must implement error recovery
- Must validate all packets

## Recovery Strategies

The protocol implements five distinct recovery strategies:

1. RECOVERY_NONE
   - Used when no recovery is possible or needed
   - Typically for non-recoverable errors or when recovery would be unsafe

2. RECOVERY_RETRY
   - Simple retry of the failed operation
   - Uses exponential backoff with jitter
   - Limited by max_retries configuration (see protocol_constants.md MAX_RETRIES)
   - Primary path: ERROR → SYNCING

3. RECOVERY_RESET_STATE
   - Resets the state machine to a known good state
   - Maintains hardware configuration
   - Used for state machine errors
   - Primary path: ERROR → IDLE

4. RECOVERY_REINIT
   - Reinitializes the affected subsystem
   - More aggressive than state reset
   - Used for hardware errors
   - Primary path: ERROR → HARDWARE_INIT or ERROR → DISPLAY_INIT

5. RECOVERY_REBOOT
   - Full system reboot
   - Most aggressive recovery option
   - Only used for fatal errors
   - Must be explicitly enabled in configuration
   - Primary path: ERROR → HARDWARE_INIT (after reboot)

Strategy selection is based on:
- Error severity (INFO, WARNING, ERROR, FATAL)
- Error category (HARDWARE, STATE, etc.)
- Error context (recoverable flag)
- System configuration (allow_reboot)

## State Definitions

### 1. HARDWARE_INIT
Initial state after power-on or reset.

#### Entry Actions
- Configure SPI interface
- Set up GPIO pins
- Initialize timers
- Configure interrupts

#### Exit Conditions
- Success → DISPLAY_INIT
- Failure → ERROR

#### Validation
- SPI configuration valid
- GPIO pins configured
- Timing requirements met

### 2. DISPLAY_INIT
Display controller initialization state.

#### Entry Actions
- Send reset sequence
- Configure display parameters
- Set up display mode
- Initialize display buffer

#### Exit Conditions
- Success → IDLE
- Failure → ERROR

#### Validation
- Reset sequence complete
- Display parameters set
- Display responds correctly

### 3. IDLE
Default protocol state, waiting for commands.

#### Entry Actions
- Clear transfer buffers
- Reset sequence counter
- Initialize packet parser

#### Exit Conditions
- SYNC received → SYNCING
- Invalid packet → ERROR

#### Validation
- Buffers empty
- No active transfers
- Parser ready

### 4. SYNCING
Establishing protocol synchronization.

#### Entry Actions
- Send/verify SYNC
- Start sync timer (see protocol_constants.md BASE_TIMEOUT_MS)
- Initialize retry counter (see protocol_constants.md MAX_RETRIES)

#### Exit Conditions
- SYNC_ACK valid → READY
- Timeout/Invalid → ERROR
- Retry needed → SYNCING

#### Validation
- Protocol version match
- Timing within limits (see protocol_constants.md for timing values)
- Retry count valid

### 5. READY
Connection established, ready for commands.

#### Entry Actions
- Reset command parser
- Initialize transfer state
- Start command timer

#### Exit Conditions
- Valid command → COMMAND_PROCESSING
- Image command → DATA_TRANSFER
- Invalid command → ERROR
- End command → IDLE

#### Validation
- Parser initialized
- Transfer state clear
- Timer active

### 6. COMMAND_PROCESSING
Processing a valid command.

#### Entry Actions
- Parse command parameters
- Validate command type
- Initialize command context

#### Exit Conditions
- Command complete → READY
- Command failed → ERROR

#### Validation
- Command type valid
- Parameters valid
- Resources available

### 7. DATA_TRANSFER
Handling image data transfer.

#### Entry Actions
- Initialize transfer buffer
- Reset chunk counter
- Start transfer timer

#### Exit Conditions
- Chunk valid → DATA_TRANSFER
- Transfer complete → READY
- Transfer failed → ERROR

#### Validation
- Buffer space available (see protocol_constants.md MAX_PACKET_SIZE)
- Sequence numbers valid
- Checksums match

### 8. ERROR
Error handling and recovery state.

#### Entry Actions
- Log error context
- Initialize recovery
- Stop active transfers

#### Exit Conditions
- Reset complete → IDLE
- Retry possible → SYNCING

#### Validation
- Error logged
- Resources cleaned up
- Recovery path valid

#### Error State Transitions
The ERROR state is a special state that:
- Can be entered from any other state
- Has well-defined exit paths (IDLE or SYNCING)
- Requires explicit recovery action
- Maintains error context during recovery

```c
// Example error state transition
bool handle_error_state(ErrorDetails *error) {
    // Must be in ERROR state
    if (get_current_state() != STATE_ERROR) {
        return false;
    }
    
    // Attempt recovery
    if (error_is_recoverable(error)) {
        // Try to recover to previous state
        return state_machine_transition(error->state, CONDITION_RECOVERED);
    }
    
    // Force reset to IDLE
    return state_machine_transition(STATE_IDLE, CONDITION_RESET);
}
```

#### Error Recovery Flow
1. State detects error condition
2. Transitions to ERROR state
3. Error handler determines recovery strategy
4. Recovery executes with timeout/retry logic
5. On success: transition to recovery target state
6. On failure: remain in ERROR state for manual intervention

## State Transition Matrix

| Current State | Event | Next State | Validation |
|--------------|-------|------------|------------|
| HARDWARE_INIT | hardware_ready | DISPLAY_INIT | Hardware configured |
| HARDWARE_INIT | init_failed | ERROR | Error context valid |
| DISPLAY_INIT | display_ready | IDLE | Display responding |
| DISPLAY_INIT | init_failed | ERROR | Error context valid |
| IDLE | sync_received | SYNCING | Valid SYNC packet |
| IDLE | invalid_packet | ERROR | Error logged |
| SYNCING | sync_validated | READY | SYNC_ACK valid |
| SYNCING | sync_failed | ERROR | Retry count valid |
| READY | valid_command | COMMAND_PROCESSING | Command valid |
| READY | image_command | DATA_TRANSFER | Transfer ready |
| READY | invalid_command | ERROR | Error logged |
| READY | end_command | IDLE | Clean shutdown |
| COMMAND_PROCESSING | command_complete | READY | Success status |
| COMMAND_PROCESSING | command_failed | ERROR | Error context |
| DATA_TRANSFER | chunk_received | DATA_TRANSFER | Chunk valid |
| DATA_TRANSFER | transfer_complete | READY | All data received |
| DATA_TRANSFER | transfer_failed | ERROR | Error logged |
| ERROR | reset_complete | IDLE | Clean reset |
| ERROR | retry_connection | SYNCING | Retry valid |
| ERROR | reinit_needed | HARDWARE_INIT/DISPLAY_INIT | Error category |
| ERROR | reboot_needed | HARDWARE_INIT | Fatal error |

## Implementation Guidelines

### 1. State Management
```c
bool transition_state(SystemState next_state, StateCondition condition) {
    // Validate transition
    if (!is_valid_transition(current_state, next_state, condition)) {
        log_error("Invalid transition");
        return false;
    }
    
    // Execute exit actions
    execute_exit_actions(current_state);
    
    // Update state
    previous_state = current_state;
    current_state = next_state;
    
    // Execute entry actions
    execute_entry_actions(next_state);
    
    // Log transition
    log_state_transition(previous_state, current_state);
    
    return true;
}
```

### 2. Error Recovery
```c
bool handle_error(ErrorContext *ctx) {
    // Log error
    log_error_context(ctx);
    
    // Determine recovery path
    RecoveryStrategy strategy = get_recovery_strategy(ctx);
    
    // Execute recovery
    switch (strategy) {
        case RECOVERY_NONE:
            return false;
            
        case RECOVERY_RETRY:
            return transition_state(SYNCING, CONDITION_RETRY);
            
        case RECOVERY_RESET_STATE:
            return transition_state(IDLE, CONDITION_RESET);
            
        case RECOVERY_REINIT:
            return transition_state(
                ctx->error_category == ERROR_CAT_DISPLAY ? 
                    STATE_DISPLAY_INIT : STATE_HARDWARE_INIT,
                CONDITION_REINIT
            );
            
        case RECOVERY_REBOOT:
            if (!config.allow_reboot) {
                return false;
            }
            system_reboot();
            return true;  // Never reached
    }
}
```

### 3. Validation
```c
bool validate_transition(SystemState current, SystemState next, StateCondition condition) {
    // Check transition matrix
    if (!is_valid_state_transition(current, next)) {
        return false;
    }
    
    // Validate condition
    if (!is_valid_condition(condition)) {
        return false;
    }
    
    // Check resources
    if (!has_required_resources(next)) {
        return false;
    }
    
    return true;
}
```

## Testing Requirements

### 1. State Transitions
- Test all valid transitions
- Verify invalid transitions are rejected
- Test error recovery paths
- Validate state changes

### 2. Error Handling
- Test timeout handling
- Verify retry logic
- Test backoff delays
- Validate error logging
- Test error recovery paths

### 3. Resource Management
- Test buffer allocation
- Verify cleanup on exit
- Test resource limits
- Validate memory usage

### 4. Performance
- Measure transition times
- Track error rates
- Monitor resource usage
- Profile critical paths
