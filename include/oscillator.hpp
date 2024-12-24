#ifndef GIML_OSCILLATOR_HPP
#define GIML_OSCILLATOR_HPP
#include "utility.hpp"
namespace giml {
    /**
     * @brief Phase Accumulator / Unipolar Saw Oscillator.
     * Can be used as a control signal and/or waveshaped into other waveforms. 
     * Will cause aliasing if sonified 
     */
    template <typename T>
    class Phasor {
    protected:
        int sampleRate;
        T phase = 0.0, frequency = 0.0, phaseIncrement = 0.0;

    public:
        Phasor() = delete;
        Phasor(int sampRate) : sampleRate(sampRate) {}
        ~Phasor() {}
        // Copy constructor
        Phasor(const Phasor<T>& c) {
            this->sampleRate = c.sampleRate;
            this->phase = c.phase;
            this->frequency = c.frequency;
            this->phaseIncrement = c.phaseIncrement;
        }
        // Copy assignment constructor
        Phasor<T>& operator=(const Phasor<T>& c) {
            this->sampleRate = c.sampleRate;
            this->phase = c.phase;
            this->frequency = c.frequency;
            this->phaseIncrement = c.phaseIncrement;
            return *this;
        }

        /**
         * @brief Increments and returns `phase` 
         * @return `phase` (after increment)
         * @todo replace wrapping with `phase -= std::floor(phase)`
         */
        virtual T processSample() {
            this->phase += this->phaseIncrement; // increment phase
            if (this->phase >= 1) { this->phase -= 1; } // if waveform zenith, wrap phase
            if (this->frequency < 0) { // if negative frequency...
                return 1.f - this->phase; // return reverse phasor
            } else { return this->phase; } // return phasor
        }

        /**
         * @brief Sets the oscillator's sample rate 
         * @param sampRate sample rate of your project
         */
        virtual void setSampleRate(int sampRate) {
            this->sampleRate = sampRate;
            this->phaseIncrement = this->frequency / static_cast<T>(this->sampleRate);
        }

        /**
         * @brief Sets the oscillator's frequency
         * @param freqHz frequency in hertz (cycles per second)
         */
        virtual void setFrequency(T freqHz) {
            this->frequency = freqHz;
            this->phaseIncrement = ::abs(this->frequency) / static_cast<T>(this->sampleRate);
        }

        /**
         * @brief Sets `phase` manually 
         * @param ph User-defined phase. 
         * Will be wrapped to the range `[0,1]` by `processSample()` 
         */
        virtual void setPhase(T ph) { // set phase manually 
            this->phase = ph;
        }
        
        /**
         * @brief Returns `phase` without incrementing. 
         * If `frequency` is negative, returns `1 - phase`
         * @return `phase` 
         */
        virtual T getPhase() {
            if (this->frequency < 0) { // if negative frequency...
                return 1 - this->phase; // return reverse phasor
            } else { return this->phase; } // return phasor
        }
    };

    /**
     * @brief Bipolar Sine Oscillator that inherits from `giml::Phasor`,
     * waveshaped with `std::sin`
     */
    template <typename T>
    class SinOsc : public Phasor<T> {
    public:
        SinOsc(int sampRate) : Phasor<T>(sampRate) {}
        
        /**
         * @brief Increments and returns `phase` 
         * @return `sin(2pi * phase)` (after increment)
         */
        T processSample() override {
            return ::sin(M_2PI * Phasor<T>::processSample());
        }
    };

    /**
     * @brief Bipolar Ideal Triangle Oscillator that inherits from `giml::Phasor`
     * Best used as a control signal, will cause aliasing if sonified 
     */
    template <typename T>
    class TriOsc : public Phasor<T> {
    public:
        TriOsc(int sampRate) : Phasor<T>(sampRate) {}

        /**
         * @brief Increments and returns `phase` 
         * @return Waveshaped `phase` (after increment)
         */
        T processSample() override {
            return ::abs(Phasor<T>::processSample() * 2 - 1) * 2 - 1;
        }
    };
}
#endif