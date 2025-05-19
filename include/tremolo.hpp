#ifndef GIML_TREMOLO_HPP
#define GIML_TREMOLO_HPP
#include <math.h>
#include "utility.hpp"
#include "oscillator.hpp"
namespace giml {
    /**
     * @brief This class implements a basic tremolo effect 
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     */
    template <typename T>
    class Tremolo : public Effect<T> {
    private:
        int sampleRate;
        Param<T> speedMillis { "speedMillis" };
        Param<T> depth { "depth" };
        giml::SinOsc<T> osc;

    public:
        // Constructor
        Tremolo() = delete;
        Tremolo (int samprate) : sampleRate(samprate), osc(samprate) {
            this->speedMillis = Param<T>("speedMillis", 1000.0, 50.0, 5000.0);
            this->osc.setFrequency(1000.0 / this->speedMillis());
            this->params.push_back(&this->speedMillis);

            this->depth = Param<T>("depth", 1.0, 0.0, 1.0);
            this->params.push_back(&this->depth);

            this->updateParams();
        }

        // Destructor
        ~Tremolo() {}

        // Copy constructor
        Tremolo(const Tremolo<T>& t) : Effect<T>(t) {
            this->sampleRate = t.sampleRate;
            this->speedMillis = t.speedMillis;
            this->depth = t.depth;
            this->osc = t.osc;
        }

        // Copy assignment operator 
        Tremolo<T>& operator=(const Tremolo<T>& t) {
            Effect<T>::operator=(t);
            this->sampleRate = t.sampleRate;
            this->speedMillis = t.speedMillis;
            this->depth = t.depth;
            this->osc = t.osc;
            return *this;
        }

        /**
         * @brief Advances oscillator and returns an enveloped version of the input
         * @param in current sample
         * @return `in` enveloped by `osc`
         */
        inline T processSample(const T& in) {
            if (!this->enabled) { return in; }
            T gain = this->osc.processSample() * 2 - 1; // waveshape SinOsc output to make it unipolar
            gain *= this->depth(); // scale by depth
            return in * (1 - gain); // return in * waveshaped SinOsc 
        }

        /**
         * @brief sets params speed and depth
         */
        void setParams(T speed = 1000.0, T depth = 1.0) {
            this->setSpeed(speed);
            this->setDepth(depth);
        }

        void updateParams() {
            this->setParams(this->speedMillis(), this->depth());
        }

        /**
         * @brief sets the rate of `osc`
         * @param millisPerCycle desired modulation frequency in milliseconds
         */
        void setSpeed(T millisPerCycle) { // set speed of LFO
            if (millisPerCycle < 0.05) { millisPerCycle = 0.05; } // osc frequency ceiling at 20kHz to avoid aliasing
            this->speedMillis = millisPerCycle;
            this->osc.setFrequency(1000.0 / this->speedMillis()); // convert to Hz (milliseconds to seconds)
        }

        /**
         * @brief sets the amount of gain reduction when `osc` is at its peak
         * @param d modulation depth (clamped to [0,1])
         */
        void setDepth(T d) { // set depth
            this->depth = d;
        }
    };
}
#endif