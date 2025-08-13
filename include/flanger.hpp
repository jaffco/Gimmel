#ifndef GIML_FLANGER_HPP
#define GIML_FLANGER_HPP
#include "utility.hpp"
#include "oscillator.hpp"

namespace giml {
    /**
     * @brief This class implements a basic flanger effect. 
     * 
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     * 
     * @todo experiment with feedback
     * 
     */
    template <typename T>
    class Flanger : public Effect<T> {
    private:
        int sampleRate;
        T depth;
        Param<T> rate { "rate", 0.0, 20.0, 0.2 };
        Param<T> depthMillis { "depthMillis", 0.0, 10.0, 5.0 };
        Param<T> blend { "blend", 0.0, 1.0, 0.5 };
        giml::CircularBuffer<T> buffer;
        giml::TriOsc<T> osc;

    public:
        // Constructor
        Flanger() = delete;

        /**
         * @brief constructor that sets max delay to 10ms, 
         * the "range end" for flange. 
         * 
         * See Effect Design Part II - Jon Dattorro 1997 Table 7 
         */
        Flanger (int samprate, T maxDepthMillis = 10.0) : sampleRate(samprate), osc(samprate) {
            // Update max range for depthMillis parameter
            this->depthMillis.setRange(0.0, maxDepthMillis);
            
            this->registerParameters(rate, depthMillis, blend);
            
            T maxDepthSamples = giml::millisToSamples(maxDepthMillis, samprate);
            this->buffer.allocate(maxDepthSamples); // max delay is 10ms

            this->updateParams();
        }

        // Destructor
        ~Flanger() {}

        // Copy constructor
        Flanger(const Flanger<T>& f) : Effect<T>(f) {
            this->sampleRate = f.sampleRate;
            this->rate = f.rate;
            this->depth = f.depth;
            this->depthMillis = f.depthMillis;
            this->blend = f.blend;
            this->buffer = f.buffer;
            this->osc = f.osc;
            this->registerParameters(rate, depthMillis, blend);
        }

        // Copy assignment operator 
        Flanger<T>& operator=(const Flanger<T>& f) {
            if (this != &f) {
                Effect<T>::operator=(f);
                this->sampleRate = f.sampleRate;
                this->rate = f.rate;
                this->depth = f.depth;
                this->depthMillis = f.depthMillis;
                this->blend = f.blend;
                this->buffer = f.buffer;
                this->osc = f.osc;
            }
            return *this;
        }

        /**
         * @brief Writes and returns sample from delay line. 
         * Return is blended with `in`
         * @param in current sample
         * @return `in` blended with past input. Changes in temporal distance 
         * from current sample result in time-varying comb filtering
         */
        inline T processSample(const T& in) override {
            // bypass behavior
            this->buffer.writeSample(in);
            if (!this->enabled) { return in; }

            // y[n] = x[n] + x[depth + osc_n * depth]
            float readIndex = this->depth + this->osc.processSample() * this->depth;
            T output = this->buffer.readSample(readIndex);
            return giml::powMix<T>(in, output, this->blend()); // return mix
        }

        /**
         * @brief sets params rate, depth, feedback and blend
         */
        void setParams(T rate = 0.20, T depth = 5.0, T blend = 0.5) {
            this->setRate(rate);
            this->setDepth(depth);
            this->setBlend(blend);
        }

        void updateParams() override {
            this->setParams(this->rate(), this->depthMillis(), this->blend());
        }

        /**
         * @brief Set modulation rate- the frequency of the LFO.  
         * @param freq frequency in Hz 
         */
        void setRate(T freq) { 
            this->rate = freq;
            this->osc.setFrequency(freq); 
        }

        /**
         * @brief Set modulation depth in milliseconds
         * @param d depth in milliseconds
         * @todo test for stability
         */
        void setDepth(T d) {
            d = giml::millisToSamples(d, this->sampleRate);
            if (2.0 * d > this->buffer.size()) { // clamp
                d = giml::samplesToMillis(this->buffer.size(), this->sampleRate) * 0.5;
            }
            this->depth = d;
        }

        /**
         * @brief Set blend 
         * @param b ratio of wet to dry (clamped to [0,1])
         */
        void setBlend(T b) { 
            this->blend = b;
        }

    };
} // namespace giml
#endif