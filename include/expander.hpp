#ifndef GIML_EXPANDER_HPP
#define GIML_EXPANDER_HPP
#include <math.h>
#include "utility.hpp"

namespace giml {
    /**
     * @brief This class implements the ideal Expander described in Reiss et al. 2011
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     */
    template <typename T>
    class Expander : public Effect<T> {
    private:
        int sampleRate;
        T thresh_dB = 0.0, ratio = 4.0, knee_dB = 2.0, 
        aRelease = 3.5, aAttack = 100.0;
        dBDetector<T> detector; // dB detector
        bool sideChainEnabled = false;
        T sideChainLastIn = 0.0;

    protected: 
        /**
         * @brief Calculates the prescribed output gain given an input, in the log domain.
         * Inversion of Reiss et al. 2011 compressor done by
         * https://www.mathworks.com/help/audio/ref/expander-system-object.html.
         * 
         * All params in dB!
         * 
         * @param x_dB input gain in dB
         * @param thresh Expander threshold
         * @param ratio Expander ratio
         * @param knee Expander knee width
         * @return `x_sc - x_dB` 
         */
        T computeGain(T x_dB, T thresh, T ratio, T knee) {
            T x_sc = x_dB; // placeholder (passthru)
            // calculate x_sc
            if (x_dB < thresh - (0.5 * knee)) { // if input < thresh - knee
                x_sc = thresh + ((x_dB - thresh) * ratio);
            } 
            else if (abs(x_dB - thresh) <= 0.5 * knee) { // if input is inside knee
                x_sc = x_dB + 
                (1.0 - ratio) *
                ( (x_dB - thresh - (knee * 0.5)) * (x_dB - thresh - (knee * 0.5)) ) /
                (2.0 * knee); // knee needs to be non-zero
            } 
            else if (x_dB > thresh + (0.5 * knee)) { // if input > thresh + knee
                x_sc = x_dB;
            }
            return x_sc; 
        }

    public:
        // Constructor
        Expander() = delete; // Do not allow an empty constructor, they must pass in a sampleRate
        Expander(int sampleRate) : sampleRate(sampleRate) {}
        
        // Destructor
        ~Expander() {}

        // Copy constructor
        Expander(const Expander<T>& c) : 
            sampleRate(c.sampleRate),
            thresh_dB(c.thresh_dB),
            ratio(c.ratio),
            knee_dB(c.knee_dB),
            aRelease(c.aRelease),
            aAttack(c.aAttack),
            detector(c.detector), // Copy detector state
            sideChainEnabled(c.sideChainEnabled),
            sideChainLastIn(c.sideChainLastIn)
        {}

        // Copy assignment operator 
        Expander<T>& operator=(const Expander<T>& c) {
            this->sampleRate = c.sampleRate;
            this->thresh_dB = c.thresh_dB;
            this->ratio = c.ratio;
            this->knee_dB = c.knee_dB;
            this->aAttack = c.aAttack;
            this->aRelease = c.aRelease;
            this->detector = c.detector; // Assign detector state
            this->sideChainEnabled = c.sideChainEnabled;
            this->sideChainLastIn = c.sideChainLastIn;
            return *this;
        }

        inline T compute(const T& in) {
            T x_dB = giml::aTodB(in); // x_dB (convert input to log domain)
            T x_sc = computeGain(x_dB, this->thresh_dB, this->ratio, this->knee_dB); // x_sc (target gain from input)
            T g_c = x_sc - x_dB; // xL (calculate difference from target gain)
            T g_s = this->detector(g_c, this->aAttack, this->aRelease); // g_s (smoothing of output gain)
            T gain = giml::dBtoA(g_s); // lin()
            return gain;
        }

        inline void toggleSideChain(bool desiredState) {
            this->sideChainEnabled = desiredState;
        }

        inline void feedSideChain(const T& in) {
            this->sideChainLastIn = in;
        }

        /**
         * @brief measures input gain and applies gain reduction
         * @param in input sample
         * @return `in` with gain reduction applied
         */
        inline T processSample(const T& in) override {
            if (!this->enabled) { return in; }
            if (this->sideChainEnabled) {
                return in * compute(this->sideChainLastIn); // apply gain reduction
            }
            return in * compute(in);
        }

        /**
         * @brief sets params threshold, ratio, knee, 
         * attack, and release
         */
        void setParams(T thresh = 0.0, T ratio = 2.0, T knee = 1.0, 
                       T attack = 3.5, T release = 100.0) {
            this->setThresh(thresh);
            this->setRatio(ratio);
            this->setKnee(knee);
            this->setAttack(attack);
            this->setRelease(release);
        }

        /**
         * @brief set threshold to trigger compression
         * @param threshdB threshold in dB
         */
        void setThresh(T threshdB) {
            this->thresh_dB = threshdB;
        }

        /**
         * @brief set compression ratio
         * @param r ratio
         */
        void setRatio(T r) {
            r = std::max(r, T(1.0 + 1e-6)); // avoid div by zero / negative values
            this->ratio = r;
        }
        
        /**
         * @brief set knee width
         * @param widthdB width value in dB
         */
        void setKnee(T widthdB) {
            widthdB = std::max(widthdB, T(1e-6)); // avoid div by zero / negative values
            this->knee_dB = widthdB;
        }

        /**
         * @brief set attack time 
         * @param attackMillis attack time in milliseconds 
         */
        void setAttack(T attackMillis) { // calculated from Reiss et al. 2011 (Eq. 7)
            attackMillis = std::max(attackMillis, T(1e-6)); // avoid div by zero / negative values
            T timeS = attackMillis * 0.001; // convert to seconds
            constexpr float log109 = 0.9542425094393249f;
            this->aAttack = exp(-log109 / (timeS * this->sampleRate));
        }

        /**
         * @brief set release time 
         * @param releaseMillis release time in milliseconds 
         */
        void setRelease(T releaseMillis) { // //
            releaseMillis = std::max(releaseMillis, T(1e-6)); // avoid div by zero / negative values
            T timeS = releaseMillis * 0.001; // convert to seconds
            constexpr float log109 = 0.9542425094393249f;
            this->aRelease = exp(-log109 / (timeS * this->sampleRate));
        }
    };
}
#endif