#ifndef OSCILLATOR_HPP
#define OSCILLATOR_HPP
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
        T phase = 0.f, frequency = 0.f, phaseIncrement = 0.f;

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
         * @brief Sets the oscillator's sample rate 
         * @param sampRate sample rate of your project
         */
        virtual void setSampleRate(int sampRate) {
            this->sampleRate = sampRate;
            this->phaseIncrement = this->frequency / static_cast<float>(this->sampleRate);
        }

        /**
         * @brief Sets the oscillator's frequency
         * @param freqHz frequency in hertz (cycles per second)
         */
        virtual void setFrequency(float freqHz) {
            this->frequency = freqHz;
            this->phaseIncrement = ::fabs(this->frequency) / static_cast<T>(this->sampleRate);
        }

        /**
         * @brief Increments and returns `phase` 
         * @return `phase` (after increment)
         */
        virtual T processSample() {
            this->phase += this->phaseIncrement; // increment phase
            if (this->phase >= 1.f) { // if waveform zenith...
                this->phase -= 1.f; // wrap phase
            }

            if (this->frequency < 0) { // if negative frequency...
                return 1.f - this->phase; // return reverse phasor
            }
            else {
                return this->phase; // return phasor
            }
        }

        /**
         * @brief Sets `phase` manually 
         * @param ph User-defined phase 
         * (will be wrapped to the range [0,1] by processSample()) 
         */
        virtual void setPhase(float ph) { // set phase manually 
            this->phase = ph;
        }
        
        /**
         * @brief Returns `phase` without incrementing. 
         * If `frequency` is negative, returns `1 - phase`
         * @return `phase` 
         */
        virtual float getPhase() {
            if (this->frequency < 0) { // if negative frequency...
                return 1.f - this->phase; // return reverse phasor
            }
            else {
                return this->phase; // return phasor
            }
        }
    };

    /**
     * @brief Bipolar Sine Oscillator that inherits from `giml::phasor`,
     * waveshaped with `sinf`
     */
    template <typename T>
    class SinOsc : public Phasor<T> {
    public:
        SinOsc(int sampRate) : Phasor<T>(sampRate) {}
        
        /**
         * @brief Increments and returns `phase` 
         * @return `sinf(phase)` (after increment)
         */
        T processSample() override {
            this->phase += this->phaseIncrement; // increment phase
            if (this->phase >= 1.f) { // if waveform zenith...
                this->phase -= 1.f; // wrap phase
            }

            if (this->frequency < 0) { // if negative frequency...
                return ::sinf(GIML_TWO_PI * (1.f - this->phase)); // return reverse phasor
            } 
            else {
                return ::sinf(GIML_TWO_PI * this->phase); // return phasor
            }
        }
    };

    /**
     * @brief Bipolar Ideal Triangle Oscillator that inherits from `giml::phasor`
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
            this->phase += this->phaseIncrement; // increment phase
            if (this->phase >= 1.f) { // if waveform zenith...
                this->phase -= 1.f; // wrap phase
            }

            if (this->frequency < 0) { // if negative frequency...
                return ::fabs((1 - this->phase) * 2 - 1) * 2 - 1; // return reverse phasor
            }
            else {
                return ::fabs(this->phase * 2 - 1) * 2 - 1; // return phasor
            }
        }
    };
}
#endif