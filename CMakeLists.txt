cmake_minimum_required(VERSION 3.13...3.27)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initialize pico-sdk from submodule
include(pico-sdk/pico_sdk_init.cmake)

project(gc9a01_display)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(
    display_test
    main.c
    GC9A01.c
)

target_link_libraries(
    display_test 
    pico_stdlib
    hardware_spi
)

# Enable USB for stdio
pico_enable_stdio_usb(display_test 1)
pico_enable_stdio_uart(display_test 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(display_test)
