# Gimmel Effects Performance Test Suite

This directory contains comprehensive benchmarking tools for all gimmel audio effects.

## Test Files

### 1. `test.cpp` - Full Audio Processing Benchmark
- Comprehensive benchmark suite that tests all gimmel effects
- Includes both `setParams()` and `processSample()` performance testing
- Tests effects processing real audio files
- Measures real-time processing capability

**Features:**
- Individual effect benchmarks for `setParams()` and `processSample()`
- Full audio processing test with effects chain
- Real-time factor calculation
- Performance comparison across all effects

### 2. `micro_benchmark.cpp` - Micro-benchmark Suite  
- Focused micro-benchmarks for each effect
- Higher iteration counts for precise measurements
- No audio file dependencies
- Quick performance profiling

**Features:**
- 100,000 iterations per processSample test
- 1,000 iterations per setParams test
- Isolated effect testing
- Minimal overhead measurements

## Effects Tested

The test suite benchmarks all 12 gimmel effects in alphabetical order:

- **Biquad** - Configurable digital filter (LPF, HPF, BPF, etc.)
- **Chorus** - Modulated delay chorus effect
- **Compressor** - Dynamic range compressor
- **Delay** - Basic delay with feedback and damping
- **Detune** - Pitch shifting detune effect
- **EnvelopeFilter** - Envelope-following filter effect
- **Expander** - Dynamic range expander  
- **Flanger** - Short delay modulation flanger
- **Phaser** - All-pass filter phaser
- **Reverb** - Schroeder reverb with customizable room parameters
- **Saturation** - Harmonic saturation/distortion
- **Tremolo** - Amplitude modulation tremolo

## Building and Running

### Quick Micro-benchmark (Recommended)
```bash
./build_micro_benchmark.sh
```

### Full Audio Processing Benchmark
```bash
./build_and_run_benchmark.sh
```

## Build Requirements

- C++17 compatible compiler (g++, clang++)
- No external dependencies (self-contained)

## Output Format

The benchmarks output performance measurements in nanoseconds:

```
GIMMEL EFFECTS MICRO-BENCHMARK
==============================
Sample Rate: 48000 Hz
Test Iterations: 100000
Test Input: 0.5

=== BIQUAD ===
         Biquad        setParams:      150 ns avg
         Biquad    processSample:       31 ns avg

=== CHORUS ===
         Chorus        setParams:       29 ns avg
         Chorus    processSample:       40 ns avg
...
```

## Performance Interpretation

- **setParams benchmarks**: Measure parameter update overhead
- **processSample benchmarks**: Measure per-sample processing cost
- **Real-time factor**: How much faster than real-time the processing runs

### Real-time Processing Guidelines

For real-time audio at 48kHz:
- Each sample must be processed in < 20,833 nanoseconds (1/48000 second)
- Lower processSample times indicate better real-time performance
- Real-time factor > 1.0 means faster than real-time processing

## Test Configuration

Default test parameters:
- Sample Rate: 48,000 Hz
- Test Input: 0.5 (mid-range signal)
- Micro-benchmark iterations: 100,000 per effect
- Audio benchmark: Uses actual audio files

## Adding New Effects

To add a new effect to the benchmark suite:

1. Add the effect header to the includes
2. Create a benchmark function following the pattern:
   ```cpp
   void benchmarkNewEffect() {
       auto effect = std::make_unique<giml::NewEffect<float>>(SAMPLE_RATE);
       // Test setParams if available
       benchmarkEffect("NewEffect", effect, TEST_INPUT);
   }
   ```
3. Call the function in main()

## Notes

- Compile with `-O3 -DNDEBUG` for accurate performance measurements
- Run multiple times and average results for consistency  
- Performance may vary based on system load and CPU features
- Some effects may not have `setParams()` methods (will be noted in output)
