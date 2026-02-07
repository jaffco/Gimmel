#include "wav.h"

// Include all gimmel effects
#include "../include/gimmel.hpp"

#include <chrono>
#include <vector>
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
const int TEST_ITERATIONS = 10000;
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

// Specialized benchmark functions for effects with different setParams signatures
void benchmarkBiquad() {
    std::cout << "\n=== BIQUAD BENCHMARK ===" << std::endl;
    auto biquad = std::make_unique<giml::Biquad<float>>(SAMPLE_RATE);
    biquad->setType(giml::Biquad<float>::BiquadUseCase::LPF_2nd);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        biquad->setParams(1000.0f, 0.707f, 0.0f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Biquad", "setParams");
    
    benchmarkEffect("Biquad", biquad, TEST_INPUT);
}

void benchmarkChorus() {
    std::cout << "\n=== CHORUS BENCHMARK ===" << std::endl;
    auto chorus = std::make_unique<giml::Chorus<float>>(SAMPLE_RATE);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        chorus->setParams(0.2f, 6.0f, 0.5f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Chorus", "setParams");
    
    benchmarkEffect("Chorus", chorus, TEST_INPUT);
}

void benchmarkCompressor() {
    std::cout << "\n=== COMPRESSOR BENCHMARK ===" << std::endl;
    auto compressor = std::make_unique<giml::Compressor<float>>(SAMPLE_RATE);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        compressor->setParams(0.0f, 2.0f, 0.0f, 2.5f, 25.0f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Compressor", "setParams");
    
    benchmarkEffect("Compressor", compressor, TEST_INPUT);
}

void benchmarkDelay() {
    std::cout << "\n=== DELAY BENCHMARK ===" << std::endl;
    auto delay = std::make_unique<giml::Delay<float>>(SAMPLE_RATE);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        delay->setParams(398.0f, 0.3f, 0.5f, 0.5f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Delay", "setParams");
    
    benchmarkEffect("Delay", delay, TEST_INPUT);
}

void benchmarkDetune() {
    std::cout << "\n=== DETUNE BENCHMARK ===" << std::endl;
    auto detune = std::make_unique<giml::Detune<float>>(SAMPLE_RATE);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        detune->setParams(1.0f, 22.0f, 0.5f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Detune", "setParams");
    
    benchmarkEffect("Detune", detune, TEST_INPUT);
}

void benchmarkEnvelopeFilter() {
    std::cout << "\n=== ENVELOPE FILTER BENCHMARK ===" << std::endl;
    auto envelopeFilter = std::make_unique<giml::EnvelopeFilter<float>>(SAMPLE_RATE);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        envelopeFilter->setParams(10.0f, 7.76f, 1105.0f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("EnvelopeFilter", "setParams");
    
    benchmarkEffect("EnvelopeFilter", envelopeFilter, TEST_INPUT);
}

void benchmarkExpander() {
    std::cout << "\n=== EXPANDER BENCHMARK ===" << std::endl;
    auto expander = std::make_unique<giml::Expander<float>>(SAMPLE_RATE);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        expander->setParams(0.0f, 2.0f, 1.0f, 2.5f, 25.0f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Expander", "setParams");
    
    benchmarkEffect("Expander", expander, TEST_INPUT);
}

void benchmarkFlanger() {
    std::cout << "\n=== FLANGER BENCHMARK ===" << std::endl;
    auto flanger = std::make_unique<giml::Flanger<float>>(SAMPLE_RATE);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        flanger->setParams(0.20f, 5.0f, 0.5f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Flanger", "setParams");
    
    benchmarkEffect("Flanger", flanger, TEST_INPUT);
}

void benchmarkPhaser() {
    std::cout << "\n=== PHASER BENCHMARK ===" << std::endl;
    auto phaser = std::make_unique<giml::Phaser<float>>(SAMPLE_RATE);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        phaser->setParams(0.5f, 0.85f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Phaser", "setParams");
    
    benchmarkEffect("Phaser", phaser, TEST_INPUT);
}

void benchmarkReverb() {
    std::cout << "\n=== REVERB BENCHMARK ===" << std::endl;
    auto reverb = std::make_unique<giml::Reverb<float>>(SAMPLE_RATE, 4, 20, 4, 2);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        reverb->setParams(0.030f, 0.6f, 0.75f, 0.5f, 1000.f, 0.75f, giml::Reverb<float>::RoomType::CUBE);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Reverb", "setParams");
    
    benchmarkEffect("Reverb", reverb, TEST_INPUT);
}

void benchmarkTremolo() {
    std::cout << "\n=== TREMOLO BENCHMARK ===" << std::endl;
    auto tremolo = std::make_unique<giml::Tremolo<float>>(SAMPLE_RATE);
    
    // Benchmark setParams
    BENCHMARK_RESET();
    for (int i = 0; i < 1000; i++) {
        BENCHMARK_START();
        tremolo->setParams(1000.0f, 1.0f);
        BENCHMARK_END_AND_RECORD();
    }
    BENCHMARK_REPORT("Tremolo", "setParams");
    
    benchmarkEffect("Tremolo", tremolo, TEST_INPUT);
}

void runFullAudioProcessingTest() {
    std::cout << "\n=== FULL AUDIO PROCESSING TEST ===" << std::endl;
    
    WAVLoader loader { "audio/Gmaj.wav" };  // Use existing audio file
    WAVWriter writer { "audio/benchmark_out.wav", loader.sampleRate };
    
    // Create all effects
    auto reverb = std::make_unique<giml::Reverb<float>>(loader.sampleRate, 4, 20, 4, 2);
    auto delay = std::make_unique<giml::Delay<float>>(loader.sampleRate);
    auto chorus = std::make_unique<giml::Chorus<float>>(loader.sampleRate);
    
    // Configure effects
    reverb->setParams(0.030f, 0.6f, 0.75f, 0.25f, 1000.f, 0.75f, giml::Reverb<float>::RoomType::CUBE);
    delay->setParams(200.0f, 0.2f, 0.3f, 0.3f);
    chorus->setParams(0.5f, 8.0f, 0.4f);
    
    reverb->enable();
    delay->enable();
    chorus->enable();
    
    float input, output;
    int sampleCount = 0;
    long long totalProcessingTime = 0;
    
    while (loader.readSample(&input)) {
        auto begin = std::chrono::steady_clock::now();
        
        // Process through effects chain
        output = input;
        output = reverb->processSample(output);
        output = delay->processSample(output);
        output = chorus->processSample(output);
        output = giml::powMix<float>(output, input, 0.5f);
        
        auto end = std::chrono::steady_clock::now();
        totalProcessingTime += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
        
        writer.writeSample(output);
        sampleCount++;
        
        if (sampleCount % 48000 == 0) {
            std::cout << "Processed " << (sampleCount / 48000) << " seconds of audio" << std::endl;
        }
    }
    
    std::cout << "Total samples processed: " << sampleCount << std::endl;
    std::cout << "Average processing time per sample: " << (totalProcessingTime / sampleCount) << " ns" << std::endl;
    std::cout << "Real-time factor: " << (1000000000.0 / (totalProcessingTime / sampleCount)) / loader.sampleRate << "x" << std::endl;
}

int main() {
    std::cout << "GIMMEL EFFECTS PERFORMANCE BENCHMARK" << std::endl;
    std::cout << "=====================================" << std::endl;
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Test Iterations: " << TEST_ITERATIONS << std::endl;
    std::cout << "Test Input: " << TEST_INPUT << std::endl;
    
    // Run individual effect benchmarks in alphabetical order
    benchmarkBiquad();
    benchmarkChorus();
    benchmarkCompressor();
    benchmarkDelay();
    benchmarkDetune();
    benchmarkEnvelopeFilter();
    benchmarkExpander();
    benchmarkFlanger();
    benchmarkPhaser();
    benchmarkReverb();
    benchmarkTremolo();
    
    // Run full audio processing test
    runFullAudioProcessingTest();
    
    return 0;
}