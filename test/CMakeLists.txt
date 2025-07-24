cmake_minimum_required(VERSION 3.12)
project(GimmelBenchmarks VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type to Release for performance testing if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler-specific optimizations for benchmarking
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        # For MSVC, remove debug flags and add optimization
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        string(REPLACE "/RTCu" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/RTCu" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG /DWIN32_LEAN_AND_MEAN")
        add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

# Include directories
include_directories(../include)
include_directories(.)  # For wav.h in current directory

# Find audio files directory
set(AUDIO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/audio")

# Micro-benchmark executable (no external dependencies)
add_executable(micro_benchmark src/micro-benchmark.cpp)

# Full benchmark executable (requires WAV loading)
add_executable(full_benchmark src/benchmark.cpp)

# Custom targets for running benchmarks
add_custom_target(run_micro
    COMMAND micro_benchmark
    DEPENDS micro_benchmark
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running micro-benchmark suite..."
)

add_custom_target(run_full
    COMMAND full_benchmark
    DEPENDS full_benchmark
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running full audio processing benchmark..."
)

add_custom_target(run_all
    DEPENDS run_micro run_full
    COMMENT "Running all benchmark suites..."
)

# Platform-specific settings
if(WIN32)
    # Windows-specific settings
    set_target_properties(micro_benchmark PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    set_target_properties(full_benchmark PROPERTIES
        WIN32_EXECUTABLE FALSE
    )
    
    # Ensure MSVC flags are properly set for each target
    if(MSVC)
        set_target_properties(micro_benchmark PROPERTIES
            COMPILE_FLAGS "/O2 /DNDEBUG"
        )
        set_target_properties(full_benchmark PROPERTIES
            COMPILE_FLAGS "/O2 /DNDEBUG"
        )
    endif()
elseif(APPLE)
    # macOS-specific settings
    set_target_properties(micro_benchmark PROPERTIES
        MACOSX_RPATH TRUE
    )
    set_target_properties(full_benchmark PROPERTIES
        MACOSX_RPATH TRUE
    )
endif()

# Optional: Enable testing with CTest
enable_testing()
add_test(NAME MicroBenchmark COMMAND micro_benchmark)
add_test(NAME FullBenchmark COMMAND full_benchmark)

# Print configuration info
message(STATUS "Gimmel Benchmarks Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Audio directory: ${AUDIO_DIR}")

# Installation (optional)
install(TARGETS micro_benchmark full_benchmark
    RUNTIME DESTINATION bin
)
