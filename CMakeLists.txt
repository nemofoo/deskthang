cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialize pico-sdk from submodule
include(pico-sdk/pico_sdk_init.cmake)

project(deskthang C CXX ASM)
pico_sdk_init()

# Enable testing
enable_testing()

# Configure stdio settings
set(PICO_ENABLE_STDIO_USB 1)
set(PICO_ENABLE_STDIO_UART 0)
set(PICO_USB_ENABLE_CDC_COMPOSITE 1)

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
    src/hardware/deskthang_gpio.c
    src/hardware/hardware.c
    src/hardware/serial.c
    src/hardware/deskthang_spi.c
    src/hardware/GC9A01.c
)

add_library(deskthang_debug
    src/debug/debug.c
)

# Add test directory
add_subdirectory(test)

# Set up include directories
include_directories(${CMAKE_SOURCE_DIR}/src)

# Link libraries
target_link_libraries(hardware 
    PRIVATE 
    error 
    pico_stdlib 
    hardware_spi
    hardware_gpio
    deskthang_debug
)

target_link_libraries(logging PRIVATE error hardware)
target_link_libraries(recovery PRIVATE error logging)
target_link_libraries(packet PRIVATE error)
target_link_libraries(command PRIVATE error packet)
target_link_libraries(protocol PRIVATE error packet command)
target_link_libraries(system PRIVATE pico_stdlib)
target_link_libraries(state 
    PRIVATE 
    error 
    system 
    logging 
    protocol 
    deskthang_debug
    pico_stdlib
    hardware_spi
    hardware_gpio
)
target_link_libraries(deskthang_debug PRIVATE error logging system)

# Main executable
add_executable(display_test
    src/main.c
)

pico_enable_stdio_usb(display_test 1)
pico_enable_stdio_uart(display_test 0)

target_link_libraries(display_test 
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
    deskthang_debug
)

pico_add_extra_outputs(display_test)
