#ifndef GIML_PHASER_HPP
#define GIML_PHASER_HPP
#include "utility.hpp"
#include "oscillator.hpp"
#include "filter.hpp"

namespace giml {
    /**
     * @brief This class implements a basic phaser effect
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     * @todo depth control (variable number of stages)
     * @todo resolve zero-delay feedback
     */
    template <typename T>
    class Phaser : public Effect<T> {
    private:
        int sampleRate;
        size_t numStages = 0;
        T rate = 0.0, feedback = 0.0, last = 0.0;
        giml::TriOsc<T> osc;
        giml::DynamicArray<giml::SVF<T>> filterbank;

    public:
        // Constructor
        Phaser() = delete;
        Phaser(int samprate, size_t stages = 6) : sampleRate(samprate), numStages(stages), osc(samprate) {
            for (size_t stage = 0; stage < numStages; stage++) {
                filterbank.pushBack(giml::SVF<T>());
            }
            this->setParams();
        }

        // Destructor
        ~Phaser() {}

        // Copy constructor
        Phaser(const Phaser<T>& p) {
            this->enabled = p.enabled;
            this->sampleRate = p.sampleRate;
            this->numStages = p.numStages;
            this->rate = p.rate;
            this->feedback = p.feedback;
            this->last = p.last;
            this->osc = p.osc;
            this->filterbank = p.filterbank;
        }

        // Copy assignment operator 
        Phaser<T>& operator=(const Phaser<T>& p) {
            this->enabled = p.enabled;
            this->sampleRate = p.sampleRate;
            this->numStages = p.numStages;
            this->rate = p.rate;
            this->feedback = p.feedback;
            this->last = p.last;
            this->osc = p.osc;
            this->filterbank = p.filterbank;
            return *this;
        }

        /**
         * @brief 
         * @param in current sample
         * @return mix of current input and last output with time-varying comb filter
         * @todo optimize SVF.setParams() call
         */
        inline T processSample(const T& in) {
            last = giml::powMix<T>(in, last, this->feedback); // ~4% CPU
            T mod = osc.processSample();

            // pass through filterbank to create phase distortion
            for (size_t stage = 0; stage < numStages; stage++) {
                auto& f = this->filterbank[stage];
                T Fc = (this->sampleRate * 0.25) / (2.0 * (numStages - stage)); // should store these
                f.setParams(Fc + mod * (Fc * 0.5), 2.0, sampleRate); // set cutoff frequency !! CPU heavy !!
                f.operator()(last); // update filter state
                last = f.allPass();
            }

            last = giml::powMix<T>(in, last); // combine with input to create comb filter effect
            if (!this->enabled) { return in; }
            return last; 
        }

        /**
         * @brief sets rate, depth, feedback
         */
        void setParams(const T& rate = 0.5, const T& feedback = 0.85) {
            this->setRate(rate);
            this->setFeedback(feedback);
        }

        /**
         * @brief Set modulation rate- the frequency of the LFO.  
         * @param freq frequency in Hz 
         */
        void setRate(const T& freq) {
            this->osc.setFrequency(freq); // set frequency in Hz
        }

        /**
         * @brief Set feedback gain. Clamped to [-1, 1].
         * @param fbGain feedback gain.
         */
        void setFeedback(const T& fbGain) { 
            this->feedback = giml::clip<T>(fbGain, -1, 1); 
        }
    };
}
#endif