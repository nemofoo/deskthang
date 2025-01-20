#!/bin/bash

# Exit on error
set -e

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