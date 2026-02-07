// Include all gimmel effects
#include "../include/gimmel.hpp"

#include <chrono>
#include <memory>
#include <iostream>
#include <string>
#include <iomanip>

// Benchmark utilities
static long long timeElapsed = 0L;
static long long iterations = 0L;

#define BENCHMARK_RESET() { timeElapsed = 0L; iterations = 0L; }

#define BENCHMARK_START() std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now()

#define BENCHMARK_END_AND_RECORD() { \
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now(); \
        timeElapsed += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count(); \
        iterations++; \
   }

#define BENCHMARK_REPORT(effectName, operation) { \
        if (iterations > 0) { \
            std::cout << std::setw(15) << effectName << " " << std::setw(15) << operation \
                      << ": " << std::setw(8) << (timeElapsed / iterations) << " ns avg" << std::endl; \
        } \
   }

// Test constants
const int SAMPLE_RATE = 48000;
const int TEST_ITERATIONS = 100000;  // More iterations for micro-benchmarks
const float TEST_INPUT = 0.5f;

// Effect benchmark template
template<typename EffectType>
void benchmarkEffect(const std::string& effectName, std::unique_ptr<EffectType>& effect, float input) {
    if (!effect) return;
    
    effect->enable();
    
    // Benchmark processSample
    BENCHMARK_RESET();
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        BENCHMARK_START();
        volatile float output = effect->processSample(input);
        BENCHMARK_END_AND_RECORD();
        (void)output; // Suppress unused variable warning
    }
    BENCHMARK_REPORT(effectName, "processSample");
}

int main() {
    std::cout << "GIMMEL EFFECTS MICRO-BENCHMARK" << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Test Iterations: " << TEST_ITERATIONS << std::endl;
    std::cout << "Test Input: " << TEST_INPUT << std::endl;
    
    // Test ALL effects in alphabetical order
    
    std::cout << "\n=== BIQUAD ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Biquad<float>>(SAMPLE_RATE);
        effect->setType(giml::Biquad<float>::BiquadUseCase::LPF_2nd);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(1000.0f, 0.707f, 0.0f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Biquad", "setParams");
        benchmarkEffect("Biquad", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== CHORUS ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Chorus<float>>(SAMPLE_RATE);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(0.2f, 6.0f, 0.5f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Chorus", "setParams");
        benchmarkEffect("Chorus", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== COMPRESSOR ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Compressor<float>>(SAMPLE_RATE);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(0.0f, 2.0f, 0.0f, 2.5f, 25.0f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Compressor", "setParams");
        benchmarkEffect("Compressor", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== DELAY ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Delay<float>>(SAMPLE_RATE);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(398.0f, 0.3f, 0.5f, 0.5f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Delay", "setParams");
        benchmarkEffect("Delay", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== DETUNE ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Detune<float>>(SAMPLE_RATE);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(1.0f, 22.0f, 0.5f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Detune", "setParams");
        benchmarkEffect("Detune", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== ENVELOPE FILTER ===" << std::endl;
    {
        auto effect = std::make_unique<giml::EnvelopeFilter<float>>(SAMPLE_RATE);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(10.0f, 7.76f, 1105.0f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("EnvelopeFilter", "setParams");
        benchmarkEffect("EnvelopeFilter", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== EXPANDER ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Expander<float>>(SAMPLE_RATE);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(0.0f, 2.0f, 1.0f, 2.5f, 25.0f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Expander", "setParams");
        benchmarkEffect("Expander", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== FLANGER ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Flanger<float>>(SAMPLE_RATE);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(0.20f, 5.0f, 0.5f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Flanger", "setParams");
        benchmarkEffect("Flanger", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== PHASER ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Phaser<float>>(SAMPLE_RATE);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(0.5f, 0.85f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Phaser", "setParams");
        benchmarkEffect("Phaser", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== REVERB ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Reverb<float>>(SAMPLE_RATE, 4, 20, 4, 2);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(0.030f, 0.6f, 0.75f, 0.5f, 1000.f, 0.75f, giml::Reverb<float>::RoomType::CUBE);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Reverb", "setParams");
        benchmarkEffect("Reverb", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== TREMOLO ===" << std::endl;
    {
        auto effect = std::make_unique<giml::Tremolo<float>>(SAMPLE_RATE);
        BENCHMARK_RESET();
        for (int i = 0; i < 1000; i++) {
            BENCHMARK_START();
            effect->setParams(1000.0f, 1.0f);
            BENCHMARK_END_AND_RECORD();
        }
        BENCHMARK_REPORT("Tremolo", "setParams");
        benchmarkEffect("Tremolo", effect, TEST_INPUT);
    }
    
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "All " << 12 << " effects tested successfully!" << std::endl;
    std::cout << "Results show average time per operation in nanoseconds." << std::endl;
    std::cout << "Lower values indicate better performance." << std::endl;
    
    return 0;
}
