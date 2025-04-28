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
        T thresh_dB = 0.f, ratio = 2.f, knee_dB = 1.f, 
        aRelease = 0.f, aAttack = 0.f;
        dBDetector<T> detector; // dB detector
        bool sideChainEnabled = false;
        T sideChainLastIn = T(0.0);

    protected: 
        /**
         * @brief Calculates the prescribed output gain given an input, in the log domain.
         * Inversion of Reiss et al. 2011 compressor done by
         * https://www.mathworks.com/help/audio/ref/expander-system-object.html.
         * 
         * All params in dB!
         * 
         * @param xG input gain
         * @param thresh Expander threshold
         * @param ratio Expander ratio
         * @param knee Expander knee width
         * @return `yG` 
         */
        float computeGain(float xG, float thresh, float ratio, float knee) {
            float yG = xG; // placeholder (passthru)
            // calculate yG
            if (xG < thresh - (0.5 * knee)) { // if input < thresh - knee
                yG = thresh + ((xG - thresh) * ratio);
            } 
            else if (2.f * abs(xG - thresh) <= knee) { // if input is inside knee
                yG = xG + 
                (1.f / ratio) *
                pow((xG - thresh) - (knee * 0.5f), 2.f) /
                (2.f * knee); // knee needs to be non-zero
            } 
            else if (xG > thresh + (0.5 * knee)) { // if input > thresh + knee
                yG = xG;
            }
          return yG; 
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
            T xG = giml::aTodB(in); // xG (convert input to log domain)
            T yG = computeGain(xG, this->thresh_dB, this->ratio, this->knee_dB); // yG (target gain from input)
            T xL = xG - yG; // xL (calculate difference from target gain)
            T yL = this->detector(xL, this->aAttack, this->aRelease); // yL (smoothing of output gain)
            T cdB = yL; // cdB = -yL
            T gain = giml::dBtoA(cdB); // lin()
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
            if (r <= 1.0) { 
                r = 1.0 + 1e-6;
                printf("Ratio must be greater than 1/n"); // necessary? 
            }
            this->ratio = r;
        }
        
        /**
         * @brief set knee width
         * @param widthdB width value in dB
         */
        void setKnee(T widthdB) {
            if (widthdB <= 0.0) {
                widthdB = 1e-6;
                printf("Knee set to pseudo-zero value, supply a positive float/n");
            }
            this->knee_dB = widthdB;
        }

        /**
         * @brief set attack time 
         * @param attackMillis attack time in milliseconds 
         */
        void setAttack(T attackMillis) { // calculated from Reiss et al. 2011 (Eq. 7)
            if (attackMillis <= 0.0) {
                attackMillis = 1e-6;
                printf("Attack time set to pseudo-zero value, supply a positive float/n");
            }
            T timeS = attackMillis * 0.001; // convert to seconds
            this->aAttack = exp(-1.0 / (timeS * this->sampleRate));
        }

        /**
         * @brief set release time 
         * @param releaseMillis release time in milliseconds 
         */
        void setRelease(T releaseMillis) { // // 
            if (releaseMillis <= 0.0) {
                releaseMillis = 1e-6;
                printf("Release time set to pseudo-zero value, supply a positive float/n");
            }
            float timeS = releaseMillis * 0.001; // convert to seconds
            this->aRelease = exp(-1.0 / (timeS * this->sampleRate));
        }
    };
}
#endif