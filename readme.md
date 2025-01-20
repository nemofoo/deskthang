# DeskThang Display Project

A desk display project using Raspberry Pi Pico (RP2040) with a GC9A01 LCD screen. The project consists of a Pico firmware and a Zig host application that can push images and patterns to the display over USB.

## Setup

Clone the repository with submodules:

```bash
git clone [repository-url]
git submodule update --init --recursive
```

Build the Pico firmware:

```bash
cd pico
mkdir build
cd build
cmake ..
make
```

Build the Zig host application:

```bash
cd host
zig build
```

## Documentation

For detailed information about the project, please refer to:
- [Main Documentation](docs/index.md)
- [Protocol Details](docs/protocol.md)
- [State Machine](docs/state_machine.md)
- [Display Information](docs/display.md)

## Hardware Setup

- Screen: GC9A01 240×240 Round LCD
- Microcontroller: Raspberry Pi Pico (RP2040)
- Connections:
  - MOSI: GPIO 19
  - SCK: GPIO 18
  - CS: GPIO 17
  - DC: GPIO 16
  - RST: GPIO 20

## Dependencies

- Pico SDK (submodule)
- Zig 0.11.0 or later
- CMake 3.13 or later
- libpng for the host application

## Project Structure

### Firmware (C)

```
src/
├── hardware/           # Hardware abstraction layer
├── protocol/          # Protocol implementation
├── state/            # State management
└── error/            # Error handling
```

### Host Application (Zig)

```
host/
├── build.zig         # Zig build configuration
├── build.zig.zon     # Zig dependency manifest
└── src/              # Source code
```
