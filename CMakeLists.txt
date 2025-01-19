cmake_minimum_required(VERSION 3.13...3.27)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initialize pico-sdk from submodule
include(pico-sdk/pico_sdk_init.cmake)

project(gc9a01_display)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add libraries
add_library(error
    src/error/error.c
)

add_library(logging
    src/error/logging.c
)

add_library(recovery
    src/error/recovery.c
)

add_library(packet
    src/protocol/packet.c
)

add_library(command
    src/protocol/command.c
)

add_library(protocol
    src/protocol/protocol.c
    src/protocol/transfer.c
)

add_library(system
    src/system/time.c
)

add_library(state
    src/state/state.c
    src/state/transition.c
    src/state/context.c
)

add_library(hardware
    src/hardware/display.c
    src/hardware/gpio.c
    src/hardware/hardware.c
    src/hardware/serial.c
    src/hardware/spi.c
    src/hardware/GC9A01.c
)

# Set up include directories for all targets
include_directories(${CMAKE_SOURCE_DIR}/src)

# Set up library dependencies
target_link_libraries(logging PRIVATE error)
target_link_libraries(recovery PRIVATE error logging)
target_link_libraries(packet PRIVATE error)
target_link_libraries(command PRIVATE error packet)
target_link_libraries(protocol PRIVATE error packet command)
target_link_libraries(system PRIVATE pico_stdlib)
target_link_libraries(state PRIVATE error system)
target_link_libraries(hardware PRIVATE error pico_stdlib hardware_spi)

set(PICO_ENABLE_STDIO_USB 1)
set(PICO_ENABLE_STDIO_UART 0)
set(PICO_USB_ENABLE_CDC_COMPOSITE 1)

add_executable(
    display_test
    src/main.c
)

target_link_libraries(
    display_test 
    pico_stdlib
    hardware_spi
    pico_stdio_usb
    error
    logging
    recovery
    packet
    command
    protocol
    state
    hardware
    system
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(display_test)

# Add test directory
add_subdirectory(test)

# Enable testing
enable_testing()
add_test(NAME unit_tests COMMAND run_tests)
