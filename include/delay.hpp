#ifndef GIML_DELAY_HPP
#define GIML_DELAY_HPP
#include "utility.hpp"
#include "filter.hpp"
namespace giml {
    /**
     * @brief This class implements a basic delay with feedback effect. 
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     * @todo store delayTime in samples 
     */
    template <typename T>
    class Delay : public Effect<T> {
    private:
        int sampleRate;
        T feedback = 0.3, delayTime = 398.0, blend = 0.5, damping = 0.5;
        giml::onePole<T> loPass; // loPass filter for damping
        giml::onePole<T> dcBlock; // See Generating Sound & Organizing Time I - Wakefield and Taylor 2022 Chapter 7 pg. 204
        giml::CircularBuffer<T> buffer; // circular buffer to store past  values

    public:
        Delay() = delete;
        Delay(int samprate, T maxDelayMillis = 3000) : sampleRate(samprate) {
            this->buffer.allocate(giml::millisToSamples(maxDelayMillis, samprate)); // max delayTime is 3 seconds
            this->loPass.setG(this->damping); // set damping 
            this->dcBlock.setCutoff(3.0, samprate);// set dcBlock at 3Hz
        }
        
        /**
         * @brief Writes and returns sample from delay line blended with input
         * @param in input sample
         * @return `in * 1-blend + y_D * blend`
         */
        inline T processSample(const T& in) {
            // calling `millisToSamples` every sample is not performant 
            T readIndex = millisToSamples(this->delayTime, this->sampleRate); // calculate read index
            T y_0 = loPass.lpf(this->buffer.readSample(readIndex)); // read from buffer and loPass
            this->buffer.writeSample(this->dcBlock.hpf(in + giml::limit<T>(y_0 * this->feedback, 0.75))); // write sample to delay buffer

          if (!(this->enabled)) { return in; } 
          return giml::linMix<float>(in, y_0, this->blend); // return wet/dry mix
        }

        /**
         * @brief sets params delayTime, feedback, damping, and blend
         */
        void setParams(T delayTime = 398.0, T feedback = 0.3, 
                       T damping = 0.5, T blend = 0.5) {
            this->setDelayTime(delayTime);
            this->setFeedback(feedback);
            this->setDamping(damping);
            this->setBlend(blend);
        }

        /**
         * @brief Set delay time
         * @param sizeMillis delay time in milliseconds. 
         * Clamped to `samplesToMillis(bufferSize)`
         */
        void setDelayTime(T sizeMillis) { 
            this->delayTime = giml::clip<T>(sizeMillis, 0, samplesToMillis(buffer.size(), this->sampleRate));
        }

        /**
         * @brief Set feedback gain.  
         * @param fbGain gain in linear amplitude. Be careful setting above 1!
         */
        void setFeedback(T fbGain) { this->feedback = fbGain; }

        /**
         * @brief Set damping manually
         * @param a damping value. Clipped to `[0,1]`
         */
        void setDamping(T a) { 
            this->damping = giml::clip<T>(a, 0, 1);;
            this->loPass.setG(this->damping);
        }

        /**
         * @brief Set blend (linear)
         * @param gWet percentage of wet to blend in. Clipped to `[0,1]`
         */
        void setBlend(T gWet) { this->blend = giml::clip<T>(gWet, 0, 1); }

        /**
         * @brief Set feedback gain based on a t60 time value  
         * @param timeMillis desired decay time in milliseconds
         */
        void setFeedback_t60(T timeMillis) {
            T normalizedDecay = millisToSamples(timeMillis, this->sampleRate) / 
            millisToSamples(this->delayTime, this->sampleRate);
            this->feedback = giml::t60<T>(static_cast<int>(::round(normalizedDecay)));
        }

    };

} // namespace giml
#endif