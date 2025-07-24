#!/bin/bash

# Build and run the gimmel effects benchmark suite
# Make sure we're in the test directory
cd "$(dirname "$0")"

echo "Building gimmel effects benchmark..."

# Compile with optimizations for performance testing
g++ -std=c++17 -O3 -DNDEBUG -I../include -o benchmark_test test.cpp

if [ $? -eq 0 ]; then
    echo "Build successful! Running benchmark..."
    echo "============================================"
    ./benchmark_test
else
    echo "Build failed!"
    exit 1
fi
