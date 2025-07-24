#!/bin/bash

# Build and run the gimmel effects micro-benchmark suite
cd "$(dirname "$0")"

echo "Building gimmel effects micro-benchmark..."

# Compile with optimizations for performance testing
g++ -std=c++17 -O3 -DNDEBUG -I../include -o micro_benchmark micro_benchmark.cpp

if [ $? -eq 0 ]; then
    echo "Build successful! Running micro-benchmark..."
    echo "============================================"
    ./micro_benchmark
else
    echo "Build failed!"
    exit 1
fi
