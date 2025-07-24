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

# Check if we're in the right directory structure
if [ ! -f "../include/gimmel.hpp" ]; then
    echo -e "${RED}✗ Error: gimmel.hpp not found in ../include/${NC}"
    echo "This script should be run from the test/ directory of the Gimmel repository."
    exit 1
fi

# Check for required tools
echo -e "${YELLOW}Checking required tools...${NC}"
missing_tools=()

if ! command -v cmake &> /dev/null; then
    missing_tools+=("cmake")
fi

if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null && ! command -v cl &> /dev/null; then
    missing_tools+=("C++ compiler (g++/clang++/MSVC)")
fi

if [ ${#missing_tools[@]} -gt 0 ]; then
    echo -e "${RED}✗ Missing required tools:${NC}"
    for tool in "${missing_tools[@]}"; do
        echo "  - $tool"
    done
    echo ""
    echo "Installation instructions:"
    echo "  Linux:   sudo apt-get install build-essential cmake"
    echo "  macOS:   brew install cmake"
    echo "  Windows: Install Visual Studio with C++ tools and CMake"
    exit 1
else
    echo -e "${GREEN}✓ All required tools found${NC}"
fi

# Check if git submodules are initialized (if this is a git repository)
if [ -d "../.git" ]; then
    echo -e "${YELLOW}Checking git submodules...${NC}"
    if git submodule status | grep -q '^-'; then
        echo -e "${RED}✗ Error: Git submodules are not initialized${NC}"
        echo "Please run the following commands first:"
        echo "  git submodule update --init --recursive"
        echo ""
        echo "Or if you're in a CI environment, make sure to checkout with submodules:"
        echo "  git clone --recursive <repo-url>"
        exit 1
    else
        echo -e "${GREEN}✓ Git submodules are properly initialized${NC}"
    fi
fi

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
    CMAKE_CONFIG_EXIT_CODE=$?
else
    cmake .. -DCMAKE_BUILD_TYPE=Release > cmake_config.log 2>&1
    CMAKE_CONFIG_EXIT_CODE=$?
    if [ $CMAKE_CONFIG_EXIT_CODE -eq 0 ]; then
        echo -e "${GREEN}✓ Configuration successful${NC}"
    else
        echo -e "${RED}✗ Configuration failed${NC}"
        echo "CMake configuration error. Common issues:"
        echo "  - Missing CMake (install with: apt-get install cmake / brew install cmake)"
        echo "  - Missing C++ compiler (install build-essential on Linux)"
        echo "  - Missing required headers (check ../include/ directory)"
        echo ""
        echo "Full error log:"
        cat cmake_config.log
        exit 1
    fi
fi

# Build
echo -e "${YELLOW}Building benchmarks...${NC}"
if [ "$VERBOSE" = true ]; then
    cmake --build . --config Release
    CMAKE_BUILD_EXIT_CODE=$?
else
    cmake --build . --config Release > cmake_build.log 2>&1
    CMAKE_BUILD_EXIT_CODE=$?
    if [ $CMAKE_BUILD_EXIT_CODE -eq 0 ]; then
        echo -e "${GREEN}✓ Build successful${NC}"
    else
        echo -e "${RED}✗ Build failed${NC}"
        echo "Build error. Common issues:"
        echo "  - Missing compiler (g++/clang++ on Linux/macOS, MSVC on Windows)"
        echo "  - Missing headers (ensure git submodules are initialized)"
        echo "  - Compiler compatibility issues"
        echo ""
        echo "Full error log:"
        cat cmake_build.log
        exit 1
    fi
fi

# Check if executables were actually built (check multiple possible locations)
micro_found=false
full_found=false

# Check for micro_benchmark in various locations
for path in "micro_benchmark" "micro_benchmark.exe" "Release/micro_benchmark.exe" "Debug/micro_benchmark.exe"; do
    if [ -f "$path" ]; then
        micro_found=true
        break
    fi
done

# Check for full_benchmark in various locations
for path in "full_benchmark" "full_benchmark.exe" "Release/full_benchmark.exe" "Debug/full_benchmark.exe"; do
    if [ -f "$path" ]; then
        full_found=true
        break
    fi
done

if [ "$micro_found" = false ]; then
    echo -e "${RED}✗ Error: micro_benchmark executable not found after build${NC}"
    echo "Build may have failed silently. Check the logs above."
    exit 1
fi

if [ "$full_found" = false ]; then
    echo -e "${RED}✗ Error: full_benchmark executable not found after build${NC}"
    echo "Build may have failed silently. Check the logs above."
    exit 1
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
    
    # First, try to build the target (this may not actually run it)
    if [ "$VERBOSE" = true ]; then
        cmake --build . --target "$target"
        local build_exit_code=$?
    else
        cmake --build . --target "$target" > "build_${target}.log" 2>&1
        local build_exit_code=$?
    fi
    
    if [ $build_exit_code -ne 0 ]; then
        echo -e "${RED}✗ Failed to prepare $description${NC}"
        if [ "$VERBOSE" = false ]; then
            echo "Build error log:"
            cat "build_${target}.log"
        fi
        echo ""
        echo "Troubleshooting tips:"
        echo "  - Ensure the executable was built successfully"
        echo "  - Check that audio files are present in audio/ directory (for full benchmark)"
        echo "  - Try running with --verbose flag for more details"
        echo "  - For Windows: ensure MSVC runtime is available"
        return 1
    fi
    
    # Now run the actual executable directly to show its output
    local executable=""
    if [ "$target" = "run_micro" ]; then
        # Check multiple possible locations for micro_benchmark
        for path in "micro_benchmark.exe" "micro_benchmark" "Release/micro_benchmark.exe" "Debug/micro_benchmark.exe"; do
            if [ -f "$path" ]; then
                executable="./$path"
                break
            fi
        done
    elif [ "$target" = "run_full" ]; then
        # Check multiple possible locations for full_benchmark
        for path in "full_benchmark.exe" "full_benchmark" "Release/full_benchmark.exe" "Debug/full_benchmark.exe"; do
            if [ -f "$path" ]; then
                executable="./$path"
                break
            fi
        done
    fi
    
    if [ -n "$executable" ] && [ -f "${executable#./}" ]; then
        echo "Executing $executable..."
        echo ""
        $executable
        local exec_exit_code=$?
        if [ $exec_exit_code -eq 0 ]; then
            echo ""
            echo -e "${GREEN}✓ $description completed successfully${NC}"
        else
            echo ""
            echo -e "${RED}✗ $description execution failed with exit code $exec_exit_code${NC}"
            return 1
        fi
    else
        echo -e "${RED}✗ Executable not found for $description${NC}"
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
echo "  ./micro_benchmark (or ./Release/micro_benchmark.exe on Windows)"
echo "  ./full_benchmark (or ./Release/full_benchmark.exe on Windows)"
echo ""
echo "CMake targets:"
echo "  cmake --build . --target run_micro  - Run micro-benchmark"
echo "  cmake --build . --target run_full   - Run full benchmark"
echo "  cmake --build . --target run_all    - Run all benchmarks"
