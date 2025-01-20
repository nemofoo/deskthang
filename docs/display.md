# Display Documentation

## Hardware Specifications

- **Display**: GC9A01 240×240 Round LCD
- **Interface**: SPI Mode 0 (CPOL=0, CPHA=0)
- **Color Format**: RGB565 (16-bit color)
- **Dimensions**: 240×240 pixels
- **Shape**: Round

## Pin Configuration

- MOSI: GPIO 19
- SCK: GPIO 18
- CS: GPIO 17
- DC: GPIO 16
- RST: GPIO 20

## Initialization Sequence

1. **Hardware Setup**
   - Configure SPI (Mode 0, MSB First)
   - Set up GPIO pins (CS, DC, RST)
   - Verify pin configuration

2. **Reset Sequence**
   - CS high, delay 5ms
   - RST low, delay 10ms
   - RST high, delay 120ms

3. **Display Configuration**
   - Power control sequence
   - Set orientation (MADCTL)
   - Configure color mode (16-bit)
   - Set gamma settings
   - Configure power control registers

4. **Display Activation**
   - Exit sleep mode (0x11)
   - 120ms delay
   - Turn display on (0x29)
   - 20ms delay

5. **Initial Display Test**
   - Clear screen to black
   - Color bars pattern (2 second display)
   - RGB gradient pattern (2 second display)
   - Checkerboard pattern (2 second display)

## Display Commands

### Core Commands
- Sleep Out (0x11): Wake display from sleep mode
- Display On (0x29): Turn display panel on
- Memory Write (0x2C): Start writing pixel data
- Column Address Set (0x2A): Set column write window
- Row Address Set (0x2B): Set row write window
- Memory Access Control (0x36): Set display orientation

### Test Patterns

1. **Color Bars**
   - 8 vertical bars showing basic colors
   - Sequence: Red, Green, Blue, Yellow, Magenta, Cyan, White, Black
   - Tests color reproduction and gamma

2. **RGB Gradient**
   - Smooth RGB gradient pattern
   - Red varies with X position
   - Green varies with Y position
   - Blue varies diagonally
   - Tests color interpolation and banding

3. **Checkerboard**
   - Alternating black and white squares
   - Square size: 20×20 pixels
   - Tests contrast and edge sharpness
   - Useful for checking display alignment

4. **Solid Color**
   - Fills entire display with single color
   - Used for basic display testing
   - Can verify uniform brightness

## State Machine Integration

The display is managed through two main states:

### HARDWARE_INIT State
- Configure SPI interface
- Set up GPIO pins
- Initialize timers
- Validate hardware configuration

### DISPLAY_INIT State
- Execute display reset sequence
- Configure display parameters
- Initialize display buffer
- Run test patterns
- Validate display operation

## Error Handling

The display implements comprehensive error checking:

1. **Hardware Validation**
   - SPI initialization check
   - GPIO pin configuration verification
   - Timing requirements validation

2. **Display Validation**
   - Reset completion verification
   - Parameter validation
   - Response checking
   - Status register monitoring

3. **Error Recovery**
   - Hardware reset capability
   - Automatic reinitialization
   - Error logging and reporting
   - State machine error handling

## Buffer Management

- Frame buffer: 240×240×2 bytes (RGB565 format)
- Buffer overflow protection
- Write window validation
- Automatic buffer cleanup

## Configuration Options

```c
// Display Configuration Structure
typedef struct {
    DisplayOrientation orientation;  // Display orientation
    uint8_t brightness;             // Display brightness (0-255)
    bool inverted;                  // Invert display colors
} DisplayConfig;

// Orientation Options
typedef enum {
    DISPLAY_ORIENTATION_0   = 0,   // 0 degrees
    DISPLAY_ORIENTATION_90  = 1,   // 90 degrees clockwise
    DISPLAY_ORIENTATION_180 = 2,   // 180 degrees
    DISPLAY_ORIENTATION_270 = 3    // 270 degrees clockwise
} DisplayOrientation;
```

## Command Interface

The display is controlled through a command-based protocol. Commands are single ASCII characters followed by optional data.

### Command Types
```c
typedef enum {
    CMD_IMAGE_START = 'I',    // Start image transfer (RGB565 format, 240×240)
    CMD_IMAGE_DATA = 'D',     // Image data chunk
    CMD_IMAGE_END = 'E',      // End image transfer
    CMD_PATTERN_CHECKER = '1', // Show checkerboard pattern
    CMD_PATTERN_STRIPE = '2',  // Show stripe pattern
    CMD_PATTERN_GRADIENT = '3',// Show gradient pattern
    CMD_HELP = 'H'            // Display help/command list
} CommandType;
```

### Command Processing

1. **Image Transfer**
   - Start with `I` command
   - Send image data in chunks
   - End with `E` command
   - Format: RGB565 (16-bit color)
   - Size: 240×240 pixels (115,200 bytes)

2. **Test Patterns**
   - `1`: Checkerboard pattern (20px squares)
   - `2`: Color bars pattern (8 vertical bars)
   - `3`: Gradient pattern (RGB interpolation)
   - Immediate execution, no additional data needed

3. **Help Command**
   - Command: `H`
   - Returns list of available commands
   - Includes command descriptions

### Command Context

Commands maintain state through a context structure:
```c
typedef struct {
    CommandType type;          // Active command
    uint32_t start_time;       // Command start timestamp
    uint32_t bytes_processed;  // Progress tracking
    uint32_t total_bytes;      // Total expected bytes
    size_t data_size;         // Current chunk size
    bool in_progress;          // Command active flag
    void *command_data;        // Command-specific data
} CommandContext;
```

### Command Status

Each command returns a status:
```c
typedef struct {
    bool success;              // Command success/failure
    uint32_t duration_ms;      // Execution time
    uint32_t bytes_processed;  // Data processed
    char message[256];         // Status message
} CommandStatus;
```

### Command Validation

Commands are validated at multiple levels:
1. **Type Validation**
   - Command character must be valid
   - Command must be appropriate for current state

2. **State Validation**
   - Commands only accepted in COMMAND_PROCESSING or DATA_TRANSFER states
   - Sequence validation for multi-packet commands

3. **Resource Validation**
   - Buffer availability check
   - Hardware readiness verification
   - Resource allocation validation

### Error Handling

1. **Command Errors**
   - Invalid command type
   - Invalid command sequence
   - Resource unavailable
   - State machine errors

2. **Transfer Errors**
   - Buffer overflow
   - Invalid data format
   - Incomplete transfer
   - Timeout conditions

3. **Recovery**
   - Command abort capability
   - Automatic state cleanup
   - Error status reporting
   - State machine recovery

### Example Usage

```bash
# Show checkerboard pattern
echo -n "1" > /dev/ttyACM0

# Show color bars
echo -n "2" > /dev/ttyACM0

# Show gradient
echo -n "3" > /dev/ttyACM0

# Show help
echo -n "H" > /dev/ttyACM0

# Transfer image (requires binary data)
echo -n "I" > /dev/ttyACM0
# [Send 115,200 bytes of RGB565 image data]
echo -n "E" > /dev/ttyACM0
```

## Performance Considerations

1. **SPI Communication**
   - Optimized write sequences
   - Minimal delays between commands
   - Efficient buffer management

2. **Drawing Operations**
   - Hardware-accelerated rectangle fills
   - Optimized pattern generation
   - Frame window optimization

3. **Timing**
   - Required delays after initialization
   - 2-second display time for test patterns
   - Command processing overhead 