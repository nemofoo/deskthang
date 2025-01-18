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

## Building the Project

1. Clone the Pico SDK (if you haven't already):
```bash
git clone https://github.com/raspberrypi/pico-sdk
