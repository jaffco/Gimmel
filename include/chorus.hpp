#ifndef GIML_CHORUS_HPP
#define GIML_CHORUS_HPP
#include <math.h>
#include "utility.hpp"
#include "oscillator.hpp"
namespace giml {
    /**
     * @brief This class implements a basic chorus effect
     * 
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     * 
     * @todo multi-layer chorus
     */
    template <typename T>
    class Chorus : public Effect<T> {
    private:
        int sampleRate;
        T rate = 0.20, depth = 0.0, offset = 0.0, blend = 0.5;
        giml::CircularBuffer<T> buffer;
        giml::TriOsc<T> osc;

    public:
        Chorus() = delete;

        /**
         * @brief constructor that sets max delay to 50ms, 
         * the border of the `transition band` where delays begin 
         * to be perceived as discrete echoes.
         * 
         * See Microsound - Curtis Roads 2004 Figure 1.1
         */
        Chorus (int samprate, float maxDepthMillis = 50.f) : sampleRate(samprate), osc(samprate) {
            this->osc.setFrequency(this->rate);
            this->depth = giml::millisToSamples(15.0, samprate);
            this->offset = giml::millisToSamples(20.0, samprate); 
            this->buffer.allocate(giml::millisToSamples(maxDepthMillis, samprate)); // max delay is 50ms 
        }

        /**
         * @brief Writes and returns sample from delay line. 
         * Return is blended with `in`
         * @param in current sample
         * @return `in` blended with past input. Changes in temporal distance 
         * from current sample create pitch-shifting via the doppler effect 
         */
        T processSample(T in) {
            // bypass behavior
            this->buffer.writeSample(in);
            if (!this->enabled) { return in; }

            // y_n = x_{n - (offset + osc_n * depth)}
            float readIndex = this->offset + this->osc.processSample() * this->depth;
            T wet = this->buffer.readSample(readIndex);
            return giml::powMix<float>(in, wet, this->blend); // return mix
        }

        /**
         * @brief sets params rate, depth and blend
         * @todo more params
         */
        void setParams(T rate = 0.20, T depth = 20.0, T blend = 0.5) {
            this->setRate(rate);
            this->setDepth(depth);
            this->setBlend(blend);
        }

        /**
         * @brief Set modulation rate- the frequency of the LFO.  
         * @param freq frequency in Hz 
         */
        void setRate(T freq) { this->osc.setFrequency(freq); }

        /**
         * @brief Set modulation depth- the delay length of 
         * excursions from an average delay (offset + depth)
         * @param d depth in milliseconds
         * @todo test for stability
         */
        void setDepth(T d) {
            d = giml::millisToSamples(d, this->sampleRate);
            this->offset = d + giml::millisToSamples(5.0, this->sampleRate);
            if (d + this->offset > this->buffer.size()) {
                d = giml::samplesToMillis(this->buffer.size(), this->sampleRate) - this->offset;
            }
            this->offset = d + giml::millisToSamples(5.0, this->sampleRate);
            this->depth = d;
        }

        /**
         * @brief Set blend 
         * @param b ratio of wet to dry (clamped to [0,1])
         */
        void setBlend(T b) { this->blend = giml::clip<T>(b, 0.f, 1.f); }

    };
}
#endif