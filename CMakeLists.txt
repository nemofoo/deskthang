cmake_minimum_required(VERSION 3.13...3.27)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initialize pico-sdk from submodule
include(pico-sdk/pico_sdk_init.cmake)

project(gc9a01_display)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

set(PICO_ENABLE_STDIO_USB 1)
set(PICO_ENABLE_STDIO_UART 0)
set(PICO_USB_ENABLE_CDC_COMPOSITE 1)

add_executable(
    display_test
    main.c
    GC9A01.c
)

target_link_libraries(
    display_test 
    pico_stdlib
    hardware_spi
    pico_stdio_usb
)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(display_test)

# Add test directory
add_subdirectory(test)

# Enable testing
enable_testing()
add_test(NAME unit_tests COMMAND run_tests)
