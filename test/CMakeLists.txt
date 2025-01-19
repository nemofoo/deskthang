cmake_minimum_required(VERSION 3.12)

# Add Unity as an external project
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG v2.5.2
)
FetchContent_MakeAvailable(unity)

# Create test executable
add_executable(run_tests
    main.c
    test_error.c
    test_logging.c
    test_recovery.c
    test_packet.c
    test_command.c
    test_protocol.c
    test_state.c
    mocks/hardware_mock.c
)

# Link against Unity and project libraries
target_link_libraries(run_tests
    unity
    error
    logging
    recovery
    packet
    command
    protocol
    state
)

# Include directories
target_include_directories(run_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${unity_SOURCE_DIR}/src
) 