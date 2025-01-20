cmake_minimum_required(VERSION 3.13...3.27)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initialize pico-sdk from submodule
include(pico-sdk/pico_sdk_init.cmake)

# Set project name and enable C/C++
project(gc9a01_display C CXX ASM)

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Configure stdio settings globally
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
    src/hardware/gpio.c
    src/hardware/hardware.c
    src/hardware/serial.c
    src/hardware/spi.c
    src/hardware/GC9A01.c
)

# Set up include directories for all targets
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/include)

# Update hardware target includes
target_include_directories(hardware
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${PICO_SDK_PATH}/src/common/pico_stdlib/include
    ${PICO_SDK_PATH}/src/common/pico_base/include
    ${PICO_SDK_PATH}/src/common/pico_binary_info/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_gpio/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_spi/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_sync/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_structs/include
    ${PICO_SDK_PATH}/src/rp2040/hardware_structs/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_irq/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_claim/include
    ${PICO_SDK_PATH}/src/rp2040/hardware_regs/include
    ${PICO_SDK_PATH}/src/rp2_common/pico_platform/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_base/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_clocks/include
    ${PICO_SDK_PATH}/src/rp2_common/hardware_dma/include
)

# Set up library dependencies
target_link_libraries(logging PRIVATE error)
target_link_libraries(recovery PRIVATE error logging)
target_link_libraries(packet PRIVATE error)
target_link_libraries(command PRIVATE error packet)
target_link_libraries(protocol PRIVATE error packet command)
target_link_libraries(system PRIVATE pico_stdlib)
target_link_libraries(state PRIVATE error system)
target_link_libraries(hardware 
    PRIVATE 
    error 
    pico_stdlib 
    hardware_spi
    hardware_gpio
    hardware_sync
    hardware_structs
    hardware_irq
    hardware_claim
)

add_executable(display_test
    src/main.c
)

# Enable stdio USB for the executable
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
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(display_test)

# Add test directory
add_subdirectory(test)

# Enable testing
enable_testing()
add_test(NAME unit_tests COMMAND run_tests)
