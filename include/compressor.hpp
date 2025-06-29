#ifndef GIML_COMPRESSOR_HPP
#define GIML_COMPRESSOR_HPP
#include <math.h>
#include "utility.hpp"
namespace giml {
    /**
     * @brief This class implements the ideal compressor described in Reiss et al. 2011
     * @tparam T floating-point type for input and output sample data such as `float`, `double`, or `long double`,
     * up to user what precision they are looking for (float is more performant)
     */
    template <typename T>
    class Compressor : public Effect<T> {
    private:
        int sampleRate;
        T thresh_dB = 0.f, ratio = 2.f, knee_dB = 1.f, 
        aRelease = 0.f, aAttack = 0.f, makeupGain_dB = 0.f;
        dBDetector<T> detector; // dB detector

    protected: 
        /**
         * @brief Applies gain reduction in the log domain
         * @param xG input gain
         * @param thresh compressor threshold
         * @param ratio compressor ratio
         * @param knee compressor knee width
         * @return `yG` 
         */
        float computeGain(float xG, float thresh, float ratio, float knee) {
            float yG = xG;
            // calculate yG
            if (2.f * (xG - thresh) < -knee) { // if input < thresh - knee
                yG = xG;
            } 
            else if (2.f * abs(xG - thresh) <= knee) { // if input is inside knee
                yG = xG + 
                (1.f / (ratio - 1.f)) *
                pow((xG - thresh) + (knee / 2.f), 2.f) /
                (2.f * knee); // knee needs to be non-zero
            } 
            else if (2.f * (xG - thresh) > knee) { // if input > thresh + knee
                yG = thresh + ((xG - thresh) / ratio);
            }
          return yG; 
        }

    public:
        // Constructor
        Compressor() = delete; // Do not allow an empty constructor, they must pass in a sampleRate
        Compressor(int sampleRate) : sampleRate(sampleRate) {}
        
        // Destructor
        ~Compressor() {}

        // Copy constructor
        Compressor(const Compressor<T>& c) {
            this->enabled = c.enabled;
            this->sampleRate = c.sampleRate;
            this->thresh_dB = c.thresh_dB;
            this->ratio = c.ratio;
            this->knee_dB = c.knee_dB;
            this->aAttack = c.aAttack;
            this->aRelease = c.aRelease;
            this->makeupGain_dB = c.makeupGain_dB;
        }

        // Copy assignment operator 
        Compressor<T>& operator=(const Compressor<T>& c) {
            this->enabled = c.enabled;
            this->sampleRate = c.sampleRate;
            this->thresh_dB = c.thresh_dB;
            this->ratio = c.ratio;
            this->knee_dB = c.knee_dB;
            this->aAttack = c.aAttack;
            this->aRelease = c.aRelease;
            this->makeupGain_dB = c.makeupGain_dB;
            return *this;
        }

        /**
         * @brief measures input gain and applies gain reduction
         * @param in input sample
         * @return `in` with gain reduction and makeup gain applied
         */
        inline T processSample(const T& in) override {
            if (!this->enabled) { return in; }
            
            T xG = giml::aTodB(in); // xG
            T yG = computeGain(xG, this->thresh_dB, this->ratio, this->knee_dB); // yG
            T xL = xG - yG; // xL
            T yL = this->detector(xL, this->aAttack, this->aRelease); // yL
            T cdB = this->makeupGain_dB - yL; // cdB = M - yL

            T gain = giml::dBtoA(cdB); // lin()
            return (in * gain); // apply gain
        }

        /**
         * @brief sets params threshold, ratio, knee, 
         * attack, release, and makeup gain
         */
        void setParams(T thresh = 0.0, T ratio = 2.0, T makeup = 0.0,
                       T knee = 1.0, T attack = 3.5, T release = 100.0) {
            this->setThresh(thresh);
            this->setRatio(ratio);
            this->setMakeupGain(makeup);
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
         * @brief set makeup gain
         * @param mdB gain value in dB.
         */
        void setMakeupGain(T mdB) {
            this->makeupGain_dB = mdB;
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
            this->aAttack = timeConstant(attackMillis, sampleRate);
        }

        /**
         * @brief set release time 
         * @param releaseMillis release time in milliseconds 
         */
        void setRelease(T releaseMillis) {
            this->aRelease = timeConstant(releaseMillis, sampleRate);
        }
    };
}
#endif