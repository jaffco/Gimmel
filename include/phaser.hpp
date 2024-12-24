#ifndef GIML_PHASER_HPP
#define GIML_PHASER_HPP
#include <math.h>
#include "utility.hpp"
#include "oscillator.hpp"
#include "biquad.hpp"
namespace giml {
    /**
     * @brief This class implements a basic phaser effect (INCOMPLETE)
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     */
    template <typename T>
    class Phaser : public Effect<T> {
    private:
        int sampleRate;
        T rate = 0.2, last = 0.0;
        giml::TriOsc<T> osc;

        size_t numStages = 6;
        giml::Biquad<T> filterbank[numStages];

    public:
        Phaser() = delete;
        Phaser (int samprate) : sampleRate(samprate), osc(samprate), filterbank{samprate, samprate, samprate, samprate, samprate, samprate} {
            this->osc.setFrequency(this->rate);
            for (auto& f : filterbank) {
                f->setType(giml::Biquad<float>::BiquadUseCase::APF_2nd);
            }
        }

        /**
         * @brief 
         * @param in current sample
         * @return 
         */
        T processSample(T in) {
            float wet = in;
            float mod = osc.processSample();

            for (int i = 0; i < this->N; i++) {
                //this->filterbank[i].setParams(Fc(i) + mod * depth(i))
                // Fc(i) = NyquistFreq / (2 * (N - i))
                // mod ranges (-1, 1)
                // depth(i) = Fc(i) * 0.5
                float Fc = (this->sampleRate * 0.5) / (2.f * (N - i));
                this->filterbank[i].setParams(Fc + mod * (Fc * 0.5f));
                wet = this->filterbank[i].processSample(wet);
            }

            return output; 
        }

        /**
         * @brief sets rate, depth, feedback
         */
        void setParams(T rate = 0.2, T depth = 1.f, T feedback = 0.707) {
            this->setRate(rate);
            this->setDepth(depth);
            this->setFeedback(feedback);
        }

        /**
         * @brief Set modulation rate- the frequency of the LFO.  
         * @param freq frequency in Hz 
         */
        void setRate(float freq) {
            this->osc.setFrequency(freq); // set frequency in Hz
        }
    };
}
#endif