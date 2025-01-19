# Protocol Documentation Cleanup - Verified Conflicts

## State Machine Recovery ✓
- [x] Document whether FALLBACK and FAIL are valid recovery strategies
  - Resolution: FALLBACK and FAIL were outdated. Replaced with comprehensive set of strategies:
    - NONE: No recovery possible/needed
    - RETRY: Simple retry with backoff
    - RESET_STATE: Reset to known good state
    - REINIT: Reinitialize subsystem
    - REBOOT: Full system reboot (if enabled)
- [x] Update state diagrams to show all recovery paths consistently
  - Verified: `protocol_architecture.md` and `protocol_state_machine.md` show same basic paths, I was incorrect about diagram differences

## Buffer & Timing Parameters ✓ 
- [x] Create a single source of truth for all protocol constants
  - Resolution: Created protocol_constants.md as single reference
  - All docs now link to this file instead of duplicating values
  - Source of truth remains protocol.h
- [x] Buffer sizes are actually consistent:
  - 512 byte max packet (`protocol.h` and `readme.md`)
  - 256 byte chunks (consistent in docs)
  - Retraction: This was not actually a conflict

## Module Organization ✓
- [ ] Choose between flat vs nested file organization
  - Verified conflict: `readme.md` shows:
    ```
    src/
    ├── hardware/           
    │   ├── hardware.c     
    │   ├── spi.c         
    │   ├── gpio.c        
    │   └── display.c     
    ```
  - While `protocol_modules.md` shows more modules but less detail

## Error Handling ✓
- [ ] Create comprehensive list of error types
  - Verified: `error.h` defines:
    ```c
    ERROR_SEVERITY_INFO,
    ERROR_SEVERITY_WARNING,
    ERROR_SEVERITY_ERROR,
    ERROR_SEVERITY_FATAL
    ```
  - Not fully documented in protocol docs

## Debug Channel ✓
- [ ] Decide on debug channel implementation
  - Verified: Only mentioned in `readme.md` under "Serial Communication"
  - Other docs don't specify debug requirements

## Recovery Persistence ✓
- [ ] Define exact requirements for error recovery persistence
  - Verified: `recovery.h` defines configuration options not mentioned in protocol docs

Retraction: Several of my earlier claims about conflicts were incorrect. The documentation is more consistent than I initially suggested, with main discrepancies being:
1. Recovery strategies (transition.h vs state machine docs)
2. Module organization detail level
3. Error handling detail level
4. Debug channel specifications
