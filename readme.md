# GC9A01 Round Display Driver for Raspberry Pi Pico

This project implements a driver for the GC9A01 1.28" round LCD display (240x240) on the Raspberry Pi Pico. The display uses SPI communication and is commonly found in smartwatch-style projects.

## Hardware Requirements

- Raspberry Pi Pico/Pico W
- 1.28" Round LCD Display with GC9A01 Controller
- Connecting wires

## Pin Connections

| Pico Pin | Display Pin |
|----------|-------------|
| GPIO 19  | MOSI (SDA) |
| GPIO 18  | SCK        |
| GPIO 17  | CS         |
| GPIO 16  | DC         |
| GPIO 20  | RST        |
| 3.3V     | VCC        |
| GND      | GND        |

## Project Structure

- `main.c` - Main application code and hardware setup
- `GC9A01.c` - Display driver implementation
- `GC9A01.h` - Driver header file with interface definitions
- `colors.h` - Color definitions in RGB565 format

## Initial Setup

1. Clone the project with submodules:
```bash
git clone --recursive [your-repo-url]
cd [project-directory]
```
2. if you forgot to clone with --recursive, just run this now to init the pico sdk
```bash
git submodule update --init
```

## Building 
```bash
# Create and enter build directory
mkdir -p build && cd build

# Clean build directory if needed
rm -rf *

# Generate build files and compile
cmake ..
make
```

## Flashing via Command Line (linux / mac)

1. Identify the pico

```bash
# Put Pico in BOOTSEL mode (hold BOOTSEL button while plugging in)
# Then list all block devices
lsblk

# Or check kernel messages for the most recently connected device
dmesg | tail
```

example output:
```bash
sda      8:0    1   1.9G  0 disk 
└─sda1   8:1    1   1.9G  0 part /media/user/RPI-RP2
```

2. Create a mount point if it doesn't exist
```bash
sudo mkdir -p /mnt/pico
```

3. Mount it
```bash
# Replace sda1 with your device (could be sdb1, sdc1, etc.)
sudo mount /dev/sda1 /mnt/pico
```

4. Copy the UF2 File to the pico
```bash
sudo cp display_test.uf2 /mnt/pico/
```

5. Unmount or suffer each time you plug it back in

```bash
sudo umount /mnt/pico
```

#### Bonus oneliner
```bash
# From build directory
rm -rf * && cmake .. && make && sudo cp display_test.uf2 /mnt/pico/
```




