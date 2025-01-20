# Test configuration
cmake_minimum_required(VERSION 3.13)

# Only build tests when not cross-compiling
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Skipping tests when cross-compiling")
    return()
endif()

project(deskthang_tests C)

# Add Unity test framework
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# Build required libraries
add_library(error
    ../src/error/error.c
)

add_library(logging
    ../src/error/logging.c
)

# Create mock libraries
add_library(mock_packet
    mocks/mock_packet.c
)

add_library(mock_time
    mocks/mock_time.c
)

add_library(mock_serial
    mocks/mock_serial.c
)

add_library(mock_protocol
    mocks/mock_protocol.c
)

add_library(mock_display
    mocks/mock_display.c
)

# Create test executables
add_executable(test_sanity
    protocol/test_sanity.c
)

add_executable(test_packet
    protocol/test_packet.c
)

add_executable(test_transfer_validation
    protocol/test_transfer_validation.c
    ../src/protocol/transfer.c
    ../src/protocol/packet.c
)

# Link Unity and project libraries
target_link_libraries(test_sanity
    unity
)

target_link_libraries(test_packet
    unity
    mock_packet
)

target_link_libraries(test_transfer_validation
    unity
    error
    logging
    mock_time
    mock_serial
    mock_protocol
    mock_display
)

# Include directories
target_include_directories(test_sanity PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${unity_SOURCE_DIR}/src
)

target_include_directories(test_packet PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${unity_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(test_transfer_validation PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${unity_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add tests
add_test(NAME test_sanity COMMAND test_sanity)
add_test(NAME test_packet COMMAND test_packet)
add_test(NAME test_transfer_validation COMMAND test_transfer_validation) 