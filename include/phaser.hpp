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
        float rate = 1.f;
        giml::TriOsc<T> osc;

        static const int N = 6;
        giml::Biquad<T> filterbank[N];

    public:
        Phaser() = delete;
        Phaser (int samprate) : sampleRate(samprate), osc(samprate), filterbank{samprate, samprate, samprate, samprate, samprate, samprate} {
            this->osc.setFrequency(this->rate);
            for (int i = 0; i < this->N; i++) {
                this->filterbank[i].setType(giml::Biquad<float>::BiquadUseCase::APF_1st);
            }
        }

        /**
         * @brief 
         * @param in current sample
         * @return 
         * TODO: 
         */
        inline T processSample(const T& in) {
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

            float output = (in * 0.5) + (wet * 0.5);  
          return output; 
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