#!/bin/bash

# Gimmel Effects Benchmark - Build and Run Script
# Cross-platform CMake build and execution

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo -e "${BLUE}Gimmel Effects Benchmark - CMake Build & Run${NC}"
echo "=============================================="

# Parse command line arguments
RUN_MICRO=false
RUN_FULL=false
RUN_ALL=false
CLEAN_BUILD=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --micro|-m)
            RUN_MICRO=true
            shift
            ;;
        --full|-f)
            RUN_FULL=true
            shift
            ;;
        --all|-a)
            RUN_ALL=true
            shift
            ;;
        --clean|-c)
            CLEAN_BUILD=true
            shift
            ;;
        --verbose|-v)
            VERBOSE=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -m, --micro     Run micro-benchmark only"
            echo "  -f, --full      Run full audio benchmark only"
            echo "  -a, --all       Run all benchmarks (default)"
            echo "  -c, --clean     Clean build directory first"
            echo "  -v, --verbose   Verbose output"
            echo "  -h, --help      Show this help"
            echo ""
            echo "Examples:"
            echo "  $0              # Build and run all benchmarks"
            echo "  $0 --micro      # Build and run micro-benchmark only"
            echo "  $0 --clean --all # Clean build, then run all"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Default to running all if no specific option given
if [ "$RUN_MICRO" = false ] && [ "$RUN_FULL" = false ]; then
    RUN_ALL=true
fi

# Build directory
BUILD_DIR="build"

# Clean build if requested
if [ "$CLEAN_BUILD" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Creating build directory...${NC}"
    mkdir "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure with CMake
echo -e "${YELLOW}Configuring with CMake...${NC}"
if [ "$VERBOSE" = true ]; then
    cmake .. -DCMAKE_BUILD_TYPE=Release
else
    cmake .. -DCMAKE_BUILD_TYPE=Release > cmake_config.log 2>&1
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Configuration successful${NC}"
    else
        echo -e "${RED}✗ Configuration failed${NC}"
        cat cmake_config.log
        exit 1
    fi
fi

# Build
echo -e "${YELLOW}Building benchmarks...${NC}"
if [ "$VERBOSE" = true ]; then
    cmake --build . --config Release
else
    cmake --build . --config Release > cmake_build.log 2>&1
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Build successful${NC}"
    else
        echo -e "${RED}✗ Build failed${NC}"
        cat cmake_build.log
        exit 1
    fi
fi

echo ""
echo -e "${GREEN}Build completed successfully!${NC}"
echo ""

# Function to run benchmark with error handling
run_benchmark() {
    local target=$1
    local description=$2
    
    echo -e "${BLUE}Running $description...${NC}"
    echo "----------------------------------------"
    
    if cmake --build . --target "$target" 2>/dev/null; then
        echo ""
        echo -e "${GREEN}✓ $description completed successfully${NC}"
    else
        echo -e "${RED}✗ $description failed${NC}"
        return 1
    fi
    echo ""
}

# Run benchmarks based on options
if [ "$RUN_ALL" = true ]; then
    echo -e "${BLUE}Running all benchmarks...${NC}"
    echo "========================================"
    run_benchmark "run_micro" "Micro-benchmark Suite"
    run_benchmark "run_full" "Full Audio Processing Benchmark"
    
elif [ "$RUN_MICRO" = true ]; then
    run_benchmark "run_micro" "Micro-benchmark Suite"
    
elif [ "$RUN_FULL" = true ]; then
    run_benchmark "run_full" "Full Audio Processing Benchmark"
fi

echo -e "${GREEN}All operations completed!${NC}"
echo ""
echo "Available executables in build directory:"
echo "  ./micro_benchmark  - Micro-benchmark suite"
echo "  ./full_benchmark   - Full audio processing benchmark"
echo ""
echo "CMake targets:"
echo "  cmake --build . --target run_micro  - Run micro-benchmark"
echo "  cmake --build . --target run_full   - Run full benchmark"
echo "  cmake --build . --target run_all    - Run all benchmarks"
