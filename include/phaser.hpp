#ifndef GIML_PHASER_HPP
#define GIML_PHASER_HPP
#include "utility.hpp"
#include "oscillator.hpp"
#include "biquad.hpp"

namespace giml {
    /**
     * @brief This class implements a basic phaser effect (BROKEN)
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     * @todo depth control (variable number of stages)
     */
    template <typename T>
    class Phaser : public Effect<T> {
    private:
        int sampleRate;
        size_t numStages = 0;
        T rate = 0.0, feedback = 0.0;
        giml::TriOsc<T> osc;
        std::vector<giml::Biquad<T>> filterbank;

    public:
        Phaser() = delete;
        Phaser(int samprate, size_t stages = 6) : sampleRate(samprate), numStages(stages), osc(samprate) {
            for (size_t stage = 0; stage < numStages; stage++) {
                auto f = giml::Biquad<T>(samprate);
                f.setType(Biquad<T>::BiquadUseCase::APF_2nd);
                f.enable();
                filterbank.push_back(f);
            }
            this->setParams();
        }

        /**
         * @brief 
         * @param in current sample
         * @return mix of current input and last output with time-varying comb filter
         */
        inline T processSample(const T& in) {

            // calculate feedback
            T output = filterbank[numStages - 1].state();
            T gamma = 1.0;
            for (size_t stage = numStages - 2; stage < numStages; stage--) {
                gamma *= filterbank[stage + 1].getG();
                output += filterbank[stage].state() * gamma;
            }
            output *= -this->feedback;
            output += in;
            output /= 1 + this->feedback * gamma * filterbank[0].getG(); 

            T mod = osc.processSample(); // get LFO value
            // pass through filterbank to create phase distortion
            for (size_t stage = 0; stage < numStages; stage++) {
                T Fc = (this->sampleRate * 0.5) / (2.0 * (numStages - stage)); // should store these
                this->filterbank[stage].setParams(Fc + mod * (Fc * 0.5));
                output = this->filterbank[stage].processSample(output); // currently broken
            }

            output = giml::powMix<T>(in, output); // combine with input to create comb filter effect
            if (!this->enabled) { return in; }
            return output; 
        }

        /**
         * @brief sets rate, depth, feedback
         */
        void setParams(T rate = 1.0, T feedback = 0.85) {
            this->setRate(rate);
            this->setFeedback(feedback);
        }

        /**
         * @brief Set modulation rate- the frequency of the LFO.  
         * @param freq frequency in Hz 
         */
        void setRate(T freq) {
            this->osc.setFrequency(freq); // set frequency in Hz
        }

        /**
         * @brief Set feedback gain. Clamped to [-1, 1].
         * @param fbGain feedback gain.
         */
        void setFeedback(T fbGain) { 
            this->feedback = giml::clip<T>(fbGain, -1, 1); 
        }
    };
}
#endif