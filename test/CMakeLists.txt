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

# Create mock libraries
add_library(mock_packet
    mocks/mock_packet.c
)

# Create test executables
add_executable(test_sanity
    protocol/test_sanity.c
)

add_executable(test_packet
    protocol/test_packet.c
)

# Link Unity and project libraries
target_link_libraries(test_sanity
    unity
)

target_link_libraries(test_packet
    unity
    mock_packet
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

# Add tests
add_test(NAME test_sanity COMMAND test_sanity)
add_test(NAME test_packet COMMAND test_packet) 