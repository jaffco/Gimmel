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
        Param<T> pitchRatio { "pitchRatio" };
        Param<T> windowSize { "windowSize" };
        Param<T> blend { "blend" };
        giml::CircularBuffer<T> buffer;
        giml::Phasor<T> osc;

    public:
        // Constructor
        Detune() = delete;
        Detune(int samprate, float maxWindowMillis = 300.0) : sampleRate(samprate), osc(samprate) {
            this->pitchRatio = Param<T>("pitchRatio", 1.0, 0.5, 2.0);
            this->params.push_back(&this->pitchRatio);
            
            this->windowSize = Param<T>("windowSize", 1000.0, 10.0, maxWindowMillis);
            this->params.push_back(&this->windowSize);
            
            this->blend = Param<T>("blend", 0.5, 0.0, 1.0);
            this->params.push_back(&this->blend);
            
            this->osc.setFrequency(1000.0 * ((1.0 - this->pitchRatio()) / this->windowSize()));
            this->buffer.allocate(giml::millisToSamples(maxWindowMillis, samprate));
        }

        // Destructor 
        ~Detune() {}

        // Copy constructor
        Detune(const Detune<T>& d) : Effect<T>(d) {
            this->sampleRate = d.sampleRate;
            this->pitchRatio = d.pitchRatio;
            this->windowSize = d.windowSize;
            this->blend = d.blend;
            this->buffer = d.buffer;
            this->osc = d.osc;
        }

        // Copy assignment operator 
        Detune<T>& operator=(const Detune<T>& d) {
            Effect<T>::operator=(d);
            this->sampleRate = d.sampleRate;
            this->pitchRatio = d.pitchRatio;
            this->windowSize = d.windowSize;
            this->blend = d.blend;
            this->buffer = d.buffer;
            this->osc = d.osc;
            return *this;
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
            if (!this->enabled) { return in; } // return input 

            T phase = this->osc.processSample(); 
            float phase2 = phase + 0.5; // mod phase
            phase2 -= floor(phase2); // wrap mod phase

            float readIndex = phase * this->windowSize(); // readpoint 1 
            float readIndex2 = phase2 * this->windowSize(); // readpoint 2

            T output = this->buffer.readSample(readIndex); // get sample
            T output2 = this->buffer.readSample(readIndex2); // get sample 2

            T windowOne = cos((phase - 0.5) * M_PI); // gain windowing
            T windowTwo = cos((phase2 - 0.5) * M_PI);// ^
            
            T out = output * windowOne + output2 * windowTwo; // windowed output
            return giml::linMix(in, out, this->blend()); 
        }

        /**
         * @brief sets params pitchRatio, windowSize, and blend
         */
        void setParams(T pitchRatio = 1.0, T windowSize = 22.0, T blend = 0.5) {
            this->setWindowSize(windowSize);
            this->setPitchRatio(pitchRatio);
            this->setBlend(blend);
        }

        /**
         * @brief Set the pitch change ratio
         * @param ratio of desired pitch to input 
         */
        void setPitchRatio(T ratio) {
            this->pitchRatio = ratio;
            this->osc.setFrequency(1000.0 * ((1.0 - ratio) / this->windowSize()));
        }

        /**
         * @brief set the maximum delay (windowSize)
         * @param sizeMillis the max amount of delay in milliseconds 
         * when `osc` is at its peak
         */
        void setWindowSize(T sizeMillis) {
            sizeMillis = giml::millisToSamples(sizeMillis, this->sampleRate);
            if (sizeMillis > this->buffer.size()) {
                sizeMillis = this->buffer.size();
            }
            this->windowSize = sizeMillis;
        }

        /**
         * @brief Set blend 
         * @param b clamped to [0,1], 1.0 = 100% wet, 0.0 = 100% dry
         */
        void setBlend(T b) { 
            this->blend = b;
        }
    };
}
#endif