#ifndef GIML_ENVELOPE_HPP
#define GIML_ENVELOPE_HPP

#include "utility.hpp"
#include "filter.hpp"

namespace giml {
    /**
     * @brief Implements a basic envelope filter
     */
    template <typename T>
    class EnvelopeFilter : public Effect<T> {
    private:
        int sampleRate;
        T aAttack, aRelease;
        Param<T> qFactor { "qFactor", 1.0, 20.0, 10.0 }; // Q factor for the filter
        Param<T> attackMillis { "attackMillis", 0.0, 100.0, 7.76 };
        Param<T> releaseMillis { "releaseMillis", 0.0, 2000.0, 1105.0 };
        Vactrol<T> mVactrol;
        SVF<T> mFilter;

      
    public:
        // Constructor
        EnvelopeFilter() = delete; // Do not allow an empty constructor, they must pass in a sampleRate
        EnvelopeFilter(int sampleRate) : sampleRate(sampleRate), 
                                         mVactrol(sampleRate), 
                                         mFilter(sampleRate) {
            this->name = "EnvelopeFilter";
            this->registerParameters(qFactor, attackMillis, releaseMillis);               
            this->updateParams();
        }
        
        // Destructor
        ~EnvelopeFilter() {}

        // Copy constructor
        EnvelopeFilter(const EnvelopeFilter<T>& e) : Effect<T>(e) {
            this->sampleRate = e.sampleRate;
            this->qFactor = e.qFactor;
            this->aAttack = e.aAttack;
            this->aRelease = e.aRelease;
            this->mVactrol = e.mVactrol;
            this->mFilter = e.mFilter;
            this->registerParameters(qFactor, attackMillis, releaseMillis);
        }

        // Copy assignment operator 
        EnvelopeFilter<T>& operator=(const EnvelopeFilter<T>& e) {
            if (this != &e) {
                Effect<T>::operator=(e);
                this->sampleRate = e.sampleRate;
                this->qFactor = e.qFactor;
                this->aAttack = e.aAttack;
                this->aRelease = e.aRelease;
                this->mVactrol = e.mVactrol;
                this->mFilter = e.mFilter;
            }
            return *this;
        }

        inline T processSample(const float& in) override {
            if (!this->enabled) { return in; }

            // rectify, then smooth with vactrol
            T rectfied = abs(in);
            T cutoff = mVactrol(rectfied);

            // "double warp"
            cutoff = std::log10((cutoff * 9.0f) + 1.0f); // basic curve 
            cutoff = std::sqrt(cutoff);  // ^0.5, general form is ^(1 / sensitivity)
            cutoff = scale(cutoff, 0, 1, 185, 3500); // map to frequency range
            
            // apply filter
            mFilter.setParams(cutoff, qFactor(), sampleRate);
            mFilter(in);
            return mFilter.loPass();
        }        
        
        // Set parameters for the envelope filter
        void setParams(T qFactor = 10.0, T attackMillis = 7.76, T releaseMillis = 1105.0) {
            this->setQ(qFactor);
            this->setAttack(attackMillis);
            this->setRelease(releaseMillis);
        }

        void updateParams() override {
            this->setParams(this->qFactor(), this->attackMillis(), this->releaseMillis());
        }

        /**
         * @brief set Q factor for the filter
         * @param qFactor Q factor, floored at 1e-6
         */
        void setQ(T qFactor) {
            qFactor = std::max(qFactor, T(1e-6));
            this->qFactor = qFactor;
        }

        /**
         * @brief set attack time 
         * @param attackMillis attack time in milliseconds 
         */
        void setAttack(T attackMillis) { // calculated from Reiss et al. 2011 (Eq. 7)
            this->attackMillis = attackMillis;
            this->aAttack = timeConstant(attackMillis, sampleRate);
        }

        /**
         * @brief set release time 
         * @param releaseMillis release time in milliseconds 
         */
        void setRelease(T releaseMillis) { // // 
            this->releaseMillis = releaseMillis;
            this->aRelease = timeConstant(releaseMillis, sampleRate);
        }

    };

} // namespace giml

#endif