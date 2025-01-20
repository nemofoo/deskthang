#!/bin/bash

# Exit on error
set -e

# Ensure we're in the project root directory
if [ ! -d "test" ] || [ ! -d "src" ]; then
    echo "Error: This script must be run from the project root directory"
    echo "Current directory: $(pwd)"
    echo "Usage: ./test/run_tests.sh"
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p build_test

# Build tests
cd build_test
cmake -DCMAKE_BUILD_TYPE=Debug ../test
make

# Run all tests
echo "Running sanity test..."
./test_sanity

echo -e "\nRunning packet tests..."
./test_packet

echo -e "\nRunning transfer validation tests..."
./test_transfer_validation

# Print summary
echo -e "\nAll tests completed!" 