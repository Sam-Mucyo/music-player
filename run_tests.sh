#!/bin/bash

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Configure with cmake
cmake ..

# Build the project and tests
echo "Building project and tests..."
make -j4

# Create test data directory
mkdir -p bin/test_data

# Run the unit tests
echo -e "\nRunning unit tests:"
./bin/run_unit_tests

# Run integration tests if they exist
if [ -f ./bin/run_integration_tests ]; then
    echo -e "\nRunning integration tests:"
    ./bin/run_integration_tests
fi

# Print test summary
echo -e "\nTest execution complete!"