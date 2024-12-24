#ifndef GIML_DETUNE_HPP
#define GIML_DETUNE_HPP
#include <math.h>
#include "utility.hpp"
#include "oscillator.hpp"
namespace giml {
    /**
     * @brief This class implements a time-domain pitchshifter 
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     * 
     * @todo calculate an optimal `windowSize` given an arbitrary `pitchRatio`
     * @todo store windowSize as samples instead of millis
     * 
     */
    template <typename T>
    class Detune : public Effect<T> {
    private:
        int sampleRate;
        T pitchRatio = 1.0, windowSize = 22.0, blend = 0.5; 
        giml::CircularBuffer<T> buffer;
        giml::Phasor<T> osc;

    public:
        Detune() = delete;
        Detune(int samprate, float maxWindowMillis = 300.0) : sampleRate(samprate), osc(samprate) {
            this->osc.setFrequency(1000.0 * ((1.0 - this->pitchRatio) / this->windowSize));
            this->buffer.allocate(giml::millisToSamples(maxWindowMillis, samprate));
        }

        /**
         * @brief Writes and returns samples from delay line.
         * @param in current sample
         * @return past input value. Changes in temporal distance from current sample
         * create pitch-shifting via the doppler effect 
         */
        inline T processSample(const T& in) {
            // bypass behavior 
            this->buffer.writeSample(in); // write sample to delay buffer
            if (!this->enabled) { return in; }

            T phase = this->osc.processSample();
            // calling `millisToSamps` every sample is not performant
            float readIndex = phase * millisToSamples(this->windowSize, this->sampleRate); // readpoint 1
            float readIndex2 = ::fmod(phase + 0.5, 1) * millisToSamples(this->windowSize, this->sampleRate); // readpoint 2

            T output = this->buffer.readSample(readIndex); // get sample
            T output2 = this->buffer.readSample(readIndex2); // get sample 2

            T windowOne = ::cosf((phase - 0.5) * M_PI); // gain windowing
            T windowTwo = ::cosf((::fmod(phase + 0.5, 1.0) - 0.5) * M_PI);// ^
            
            T out = output * windowOne + output2 * windowTwo; // windowed output
            return giml::powMix(in, out, this->blend);
        }

        /**
         * @brief sets params pitchRatio and windowSize
         */
        void setParams(T pitchRatio = 1.0, T windowSize = 22.0, T blend = 0.5) {
            this->setPitchRatio(pitchRatio);
            this->setWindowSize(windowSize);
            this->setBlend(blend);
        }

        /**
         * @brief Set the pitch change ratio
         * @param ratio of desired pitch to input 
         */
        void setPitchRatio(T ratio) {
            this->pitchRatio = ratio;
            this->osc.setFrequency(1000.0 * ((1.0 - ratio) / this->windowSize));
        }

        /**
         * @brief set the maximum delay (windowSize)
         * @param sizeMillis the max amount of delay in milliseconds 
         * when `osc` is at its peak
         */
        void setWindowSize(T sizeMillis) {
            if (sizeMillis > giml::samplesToMillis(this->buffer.size(), this->sampleRate)) {
                sizeMillis = giml::samplesToMillis(this->buffer.size(), this->sampleRate);
            }
            this->windowSize = sizeMillis;
        }

        /**
         * @brief Set blend 
         * @param b ratio of wet to dry (clamped to [0,1])
         */
        void setBlend(T b) { this->blend = giml::clip<T>(b, 0.0, 1.0); }
    };
}
#endif