#ifndef GIML_REVERB_HPP
#define GIML_REVERB_HPP

#include "utility.hpp"

namespace giml {

    /**
     * @brief 
     * 
     * @tparam T 
     * 
     * 
     * setRoom depends on setTime, so make sure to call setTime first or else parameters will not update correctly
     */
    template <typename T>
    class Reverb : public Effect<T> {
        //Schroeder implementation of reverb (4 comb + 2 APF)?
    private:
        //The three user-defined parameters
        float param__time = 0.f; //Controls D for delay lines (in ms)
        float param__damping = 0.f; //controls feedback loop gains
        float param__length = 1.f; // controls volume of room and decay time of signal

        CircularBuffer<T> delayLineInput; //Store all input history
        int sampleRate;

        //Class forward declarations (definitions down below)
        template <typename T>
        class APF;

        template <typename T>
        class CombFilter;

        //Parallel comb filters array
        int numCombFilters = 20;
        DynamicArray<CombFilter<T>> parallelCombFilters;

        CircularBuffer<T> delayLineSummedCombFilterOutput;

        //Series APF arrays (one for before the comb filters and one for after)
        int numBeforeAPFs = 0, numAfterAPFs = 0;
        DynamicArray<APF<T>> beforeAPFs, afterAPFs;
    
    public:
        //Constructor
        Reverb() = delete;
        Reverb(int sampleRate) : sampleRate(sampleRate) {
            this->delayLineInput.allocate(sampleRate * 5); //Support a maximum of 5 seconds of reverb
            this->delayLineSummedCombFilterOutput.allocate(sampleRate * 5);

            const CircularBuffer<T>* pCommonDelayLineIn;
            if (numBeforeAPFs > 0) {
                this->beforeAPFs.pushBack(APF<T>(sampleRate, &this->delayLineInput));
                for (int i = 1; i < numBeforeAPFs; i++) {
                    this->beforeAPFs.pushBack(APF<T>(sampleRate, this->beforeAPFs[i - 1].pDelayLineOut));
                }
                pCommonDelayLineIn = this->beforeAPFs[numBeforeAPFs - 1].pDelayLineOut;
            }
            else {
                pCommonDelayLineIn = &this->delayLineInput; //support last before APF's output
            }
            
            for (int i = 0; i < numCombFilters; i++) {
                //Since all comb filters are in parallel, they'll use the same delay line input
                this->parallelCombFilters.pushBack(CombFilter<T>(sampleRate, pCommonDelayLineIn)); //Initialize the n comb filters
                //Comb filters are altered in phase when feedback gains are set in `.setRoom()`
            }

            if (this->numAfterAPFs > 0) {
                this->afterAPFs.pushBack(APF<T>(sampleRate, &delayLineSummedCombFilterOutput));
                for (int i = 1; i < numAfterAPFs; i++) {
                    this->afterAPFs.pushBack(APF<T>(sampleRate, this->afterAPFs[i - 1].pDelayLineOut));
                }
            }
            

        }
        //Copy constructor
        Reverb(const Reverb<T>& r) {
            this->sampleRate = r.sampleRate;
            this->delayLineInput = r.delayLineInput; //Deep copies the circular buffer already

            this->numCombFilters = r.numCombFilters;
            this->beforeAPFs = r.beforeAPFs;
            this->afterAPFs = r.afterAPFs;

            this->parallelCombFilters = r.parallelCombFilters;
            this->beforeAPFs = r.beforeAPFs;
            this->afterAPFs = r.afterAPFs;

        }
        Reverb<T>& operator=(const Reverb<T>& r) {
            this->sampleRate = r.sampleRate;
            this->delayLineInput = r.delayLineInput; //Deep copies the circular buffer already

            this->numCombFilters = r.numCombFilters;
            this->beforeAPFs = r.beforeAPFs;
            this->afterAPFs = r.afterAPFs;

            this->parallelCombFilters = r.parallelCombFilters;
            this->beforeAPFs = r.beforeAPFs;
            this->afterAPFs = r.afterAPFs;

            return *this;
        }
        void setTime(float t) { //in ms
            this->param__time = t;
            // Recalculate/set the delay indices

            /**
             * @brief Overview of calculating delay indices for N comb filters
             * 
             * Requirements:
             * - minDelay:maxDelay = 1:1.5 ratio
             * - delays share no easy common factors so that we can fill the gap in between
             * 
             * User is providing us with maxDelay directly, and we can directly calculate the minDelay
             * since we have the 1.5 ratio restriction
             * 
             * For all the other comb filters in between, we need to somehow bridge the gap
             * parametrically(w/ respect to `t`)/smoothly to avoid accidental overlap in the
             * impulse response (we want to fill in all the gaps in between and we want the
             * comb filters all linearly independent from one another)
             * 
             * Since minDelay = maxDelay/1.5 = 2/3 maxDelay, our range is from 0.666maxDelay to 1maxDelay.
             * This gives us a range of 1/3 to pick from. We can't use a linear interpolation because we
             * want an easy way to split the space evenly and get out semi-uneven values (that are hopefully
             * linearly independent).
             * 
             * I originally looked to 1/3sin() from [0, pi/4] ([0,1/3] in the range), but the distribution
             * of values seemed heavily skewed to >0.5. Switching to tangential interpolation seemed smarter
             * since the distribution seemed more even. Since we already have max and min delay indices,
             * we only need everything in between. With `numCombFilters - 2` delay indices left to find, we need
             * to divide the [0, pi/4] range of tangent into `numCombFilters - 2 + 1` spaces and use those. Those
             * multipliers seem linearly independent enough and smooth enough.
             * 
             * The tangent will only give us the portion of the 1/3rd we are at so we need to add on the
             * additional 2/3 we start from -> (1/3tan(n (pi/4) /numFilters) + 2/3) * maxDelay
             * 
             * Simplify to (tan + 2)/3 * maxDelay (less operations, same result)
             * 
             */

            //TODO: Need to interpolate indices instead of rounding them
            int* delayIndices = (int*)calloc(this->numCombFilters, sizeof(int));
            //int delayIndices[this->numCombFilters] = {0};
            delayIndices[0] = ::round(this->sampleRate * this->param__time); //They give us max
            delayIndices[this->numCombFilters - 1] = ::round(delayIndices[0] / 1.5f); //We know min because of the ratio restriction
            float division = M_PI_4 / (this->numCombFilters - 1); // (pi/4)/number of intermediate comb filters we have left
            for (int i = 1; i < this->numCombFilters - 1; i++) { //Fill in the rest of the comb filters, order does not matter
                delayIndices[i] =  delayIndices[0] * (::tanf(i * division) + 2) / 3; //maxDelay * intermediate multiplier
            }
            //Actually set their new delay indices
            for (int i = 0; i < this->numCombFilters; i++) {
                this->parallelCombFilters[i].setDelayIndex(delayIndices[i]);
            }

            //TODO: Do what we need to do for APF

        }
        void setDamping(float damping) { // [0, 1) (non-inclusive because we need gain to be decaying for BIBO stability)
            if (damping < 0) {
                damping = 0;
            }
            else if (damping >= 1) {
                damping = 0.999999;
            }
            this->param__damping = damping;
            // Recalculate the g value from RT-60 and new damping

            /**
             * @brief Overview of feedback gain calculations
             * lpf_G = g(1-comb_G) where they give us `g`: [0, 1)
             * 
             */

            //Set the LPF feedback gains
            for (int i = 0; i < this->numCombFilters; i++) {
                float g = damping*(1-this->parallelCombFilters[i].getCombFeedbackGain());
                parallelCombFilters[i].setLPFFeedbackGain(g);
            }

            //TODO: Do what we need to do for APF

        }
        enum class RoomType {
            CUBE, SPHERE //TODO: Add more/better later
        };

        void setRoom(float length, RoomType type = RoomType::SPHERE, float absorptionCoefficient = 0.75f) {
            //Length in feet (ft)
            if (length < 0) {
                length = 0;
            }
            this->param__length = length;
            // recalculate the RT-60 decay time and the comb filter gains

            /**
             * @brief Overview of "room" acoustics
             * Reverb is supposed to make it sound like a bunch of echoes bouncing off of
             * walls (hence delay lines)
             * 
             * We will somewhat model a "room" that has some volume, some surface area (related
             * to each other in ways that signify different room shapes) and an average absorption
             * coefficient
             * 
             * absorptionCoeff = 1 represents that the sound was completely absorbed by the room
             * 
             * TODO: Find out more about absorptionCoefficient defaults
             * 
             * This equation gives us decay time of the signal:
             * RT-60 = V/(2 * SA * absorptionCoefficient)
             * volume (ft^3), surface area (ft^2), 
             * 
             * Some basic shapes:
             * Cube: V = s^3, SA = 6s^2
             * Sphere: V = 4/3 pi r^3, SA = 4 pi r^2
             */
            float RT60 = 0.f;
            switch (type) {
                //Simplified V/SA formulas:
                case RoomType::SPHERE: {
                    RT60 = length / (6 * absorptionCoefficient);
                    break;
                }
                case RoomType::CUBE: {
                    RT60 = length / (12 * absorptionCoefficient);
                    break;
                }
            }

            /**
             * @brief 
             * 
             * Calculating the comb filter feedback gain follows the equation:
             * g = 10^{\frac{3D}{RT60 * sampleFreq}}
             */

            //Set comb feedback gains corresponding to the newly calculated RT60 decay time
            for (int i = 0; i < this->numCombFilters; i++) {
                int delayIndex = this->parallelCombFilters[i].getDelayIndex();
                float feedbackGain = ::powf(10, -3 * delayIndex / (this->sampleRate * RT60));
                //Flip the phase of every other comb filter
                if (i % 2) {
                    this->parallelCombFilters[i].setCombFeedbackGain(feedbackGain);
                }
                else {
                    this->parallelCombFilters[i].setCombFeedbackGain(-feedbackGain);
                }
                
            }

            //TODO: Do what we need to do for APF

        }
        
        T processSample(T in) {
            this->delayLineInput.writeSample(in);
            if (!(this->enabled)) {
                return in;
            }
            T prev = in;
            if (this->numBeforeAPFs > 0) {
                for (auto& apf : this->beforeAPFs) {
                    prev = apf.processSample(prev);
                }
            }
            // And then the comb filters
            T summedValue = 0;
            for (auto& combFilter : this->parallelCombFilters) {
                summedValue += combFilter.processSample(prev);
            }
            //And then finally insert summedValue into the last set of comb filters
            if (this->numAfterAPFs > 0) {
                for (auto& apf : this->afterAPFs) {
                    summedValue = apf.processSample(summedValue);
                }
            }
            return summedValue;
        }
    private:
        
        template <typename T>
        class APF { //not the same as 2nd order APF present in Biquad since this is Nth-order
            /* Have APF expose out delay line so that the next stage can take from there */
        public:
            const CircularBuffer<T>* pDelayLineOut; //Make const pointer so that it's read-only
            //Constructor
            APF() = delete;
            APF(int sampleRate, const CircularBuffer<T>* pDelayLineIn) : sampleRate(sampleRate), pDelayLineIn(pDelayLineIn) {
                this->delayLineOut.allocate(sampleRate * 5);
                this->pDelayLineOut = &this->delayLineOut; //Publicize out pointer for Reverb to chain components together
            }
            //Copy constructor
            APF(const APF<T>& a) {
                this->sampleRate = a.sampleRate;
                this->pDelayLineIn = a.pDelayLineIn; //Use the same delay line in
                this->delayLineOut = a.delayLineOut;
                this->pDelayLineOut = &this->delayLineOut; //Publicize out pointer for Reverb to chain components together
            }
            //Copy assignment constructor
            APF<T>& operator=(const APF<T>& a) {
                this->sampleRate = a.sampleRate;
                this->pDelayLineIn = a.pDelayLineIn; //Use the same delay line in
                this->delayLineOut = a.delayLineOut;
                this->pDelayLineOut = &this->delayLineOut; //Publicize out pointer for Reverb to chain components together

                return *this;
            }

            T processSample(T in) {
                return in;
            }
        private:
            const CircularBuffer<T>* pDelayLineIn; //Make const pointer so that it's read-only
            CircularBuffer<T> delayLineOut;
            int sampleRate;
        };

        template <typename T>
        class CombFilter { //not necessarily a standalone effect in itself
        private:
            const CircularBuffer<T>* pDelayLineX; //Const pointer to avoid changing the delay line, we only want to read from it
            CircularBuffer<T> delayLineY;
            float CombFeedbackGain, LPFFeedbackGain;
            int delayIndex;

        public:
            //Constructor
            CombFilter() = delete;
            CombFilter(int sampleRate, const CircularBuffer<T>* pDelayLineIn, int delayIndex=0, float combFeedbackGain=0.f, float lpfFeedbackGain=0.f) : pDelayLineX(pDelayLineIn), delayIndex(delayIndex), CombFeedbackGain(combFeedbackGain), LPFFeedbackGain(lpfFeedbackGain) {
                this->delayLineY.allocate(sampleRate * 5);
            }
            //Copy constructor
            CombFilter(const CombFilter& c) {
                this->pDelayLineX = c.pDelayLineX;
                this->delayLineY = c.delayLineY;
                this->delayIndex = c.delayIndex;
                this->CombFeedbackGain = c.CombFeedbackGain;
                this->LPFFeedbackGain = c.LPFFeedbackGain;
            }
            //Copy assignment operator
            CombFilter& operator=(const CombFilter& c) {
                this->pDelayLineX = c.pDelayLineX;
                this->delayLineY = c.delayLineY;
                this->delayIndex = c.delayIndex;
                this->CombFeedbackGain = c.CombFeedbackGain;
                this->LPFFeedbackGain = c.LPFFeedbackGain;

                return *this;
            }
            void setDelayIndex(int delayIndex) {
                this->delayIndex = delayIndex;
            }

            int getDelayIndex() const {
                return this->delayIndex;
            }

            void setCombFeedbackGain(float g) {
                this->CombFeedbackGain = g;
            }

            float getCombFeedbackGain() const {
                return this->CombFeedbackGain;
            }

            void setLPFFeedbackGain(float g) {
                this->LPFFeedbackGain = g;
            }

            T processSample(T in) {
                // x[n-D] - g2 x[n-D-1] + g2 y[n-1] + g1 y[n-D]  (g2 = LPF gain)
                // x[n-D] + g2 (y[n-1] - x[n-(D+1)]) + g1 y[n-D]
                T returnVal = this->pDelayLineX->readSample(delayIndex) // x[n-D]
                    + LPFFeedbackGain * (this->delayLineY.readSample(1) - this->pDelayLineX->readSample(delayIndex+1)) // g2 (y[n-1] - x[n-(D+1)])
                    + CombFeedbackGain * this->delayLineY.readSample(delayIndex); // g1 y[n-D]
                this->delayLineY.writeSample(returnVal);
                return returnVal;
            }

        };



    };
}


#endif