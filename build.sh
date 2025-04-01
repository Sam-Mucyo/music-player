#!/bin/bash

# Create build directory
mkdir -p build
cd build

# Configure with cmake
cmake ..

# Build
make -j4

# Message
echo ""
echo "Build complete. Binaries are in build/bin directory."
echo "To run the server: ./bin/music_server [port] [music_directory]"
echo "To run the client: ./bin/music_client [server_host] [port]"