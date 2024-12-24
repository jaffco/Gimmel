#ifndef GIML_REVERB_HPP
#define GIML_REVERB_HPP
#include "utility.hpp"
#include "oscillator.hpp"
#include "biquad.hpp"
namespace giml {

    /**
     * @brief Reverb Effect
     * 
     * Implements a Schroeder reverb (20 combs + 4 nested APFs)
     * 
     * @tparam T floating-point (float or double or long double)
     * 
     */
    template <typename T>
    class Reverb : public Effect<T> {
    private:
        // The user-defined parameters
        float param__time = 0.f; //Controls D for delay lines (in ms)
        float param__regen = 0.f; //controls LPF feedback gains for Comb Filter
        float param__damping = 0.f; //controls feedback loop gains for APF
        float param__length = 1.f; // controls volume of room and decay time of signal
        float param__blend = 0.5f;

        int sampleRate;

        // Class forward declarations (definitions down below)
        template <typename U>
        class NestedAPF;

        template <typename U>
        class CombFilter;

        // Parallel comb filters array
        int numCombFilters;
        DynamicArray<CombFilter<T>> parallelCombFilters;

        // Series APF arrays (one for before the comb filters and one for after)
        int numBeforeAPFs, numAfterAPFs;
        DynamicArray<NestedAPF<T>*> beforeAPFs, afterAPFs;
        NestedAPF<T>* createNestedAPF(int sampleRate, int nestingDepth = 0) { // Uses `new`, must be properly deallocated in the Destructor
            NestedAPF<T>* pCurrentAPF = nullptr;
            for (int i = 0; i < nestingDepth + 1; i++) {
                // Using placement `new` to force invocation of malloc instead for when we might want to go into embedded
                NestedAPF<T>* temp = (NestedAPF<T>*)malloc(sizeof(NestedAPF<T>)); // Need to use temp or else will lead to infinite nesting
                NestedAPF<T>* n = new (temp) NestedAPF<T>{ sampleRate, pCurrentAPF };
                pCurrentAPF = temp;
            }
            return pCurrentAPF;
        }
    
    public:
        //Constructor - creates all APFs/Comb Filters and puts them in place
        Reverb() = delete;
        Reverb(int sampleRate, int numBeforeAPFs = 2, int numCombFilters = 20, int numAfterAPFs = 2, int APFNestingDepth = 2) : sampleRate(sampleRate),
        numBeforeAPFs(numBeforeAPFs), numCombFilters(numCombFilters), numAfterAPFs(numAfterAPFs) {
            for (int i = 0; i < numBeforeAPFs; i++) {
                this->beforeAPFs.pushBack(this->createNestedAPF(sampleRate, APFNestingDepth)); //Let's try nesting depth of 1 first
            }
            
            for (int i = 0; i < numCombFilters; i++) {
                // Since all comb filters are in parallel, they'll use the same delay line input (?)
                this->parallelCombFilters.pushBack(CombFilter<T>(sampleRate, (i%2))); // Initialize the n comb filters
                // Comb filters are altered in phase when feedback gains are set in `.setRoom()`
            }

            for (int i = 0; i < numAfterAPFs; i++) {
                this->afterAPFs.pushBack(this->createNestedAPF(sampleRate, 2));
            }
            
        }

        // Copy constructor
        Reverb(const Reverb<T>& r) {
            this->sampleRate = r.sampleRate;

            this->param__time = r.param__time;
            this->param__regen = r.param__regen;
            this->param__length = r.param__length;
            this->param__damping = r.param__damping;

            this->numCombFilters = r.numCombFilters;
            this->numBeforeAPFs = r.numBeforeAPFs;
            this->numAfterAPFs = r.numAfterAPFs;

            this->parallelCombFilters = r.parallelCombFilters;
            this->beforeAPFs = r.beforeAPFs;
            this->afterAPFs = r.afterAPFs;

        }

        // Copy assignment constructor
        Reverb<T>& operator=(const Reverb<T>& r) {
            this->sampleRate = r.sampleRate;
            
            this->param__time = r.param__time;
            this->param__regen = r.param__regen;
            this->param__length = r.param__length;
            this->param__damping = r.param__damping;

            this->numCombFilters = r.numCombFilters;
            this->numBeforeAPFs = r.numBeforeAPFs;
            this->numAfterAPFs = r.numAfterAPFs;

            this->parallelCombFilters = r.parallelCombFilters;
            this->beforeAPFs = r.beforeAPFs;
            this->afterAPFs = r.afterAPFs;

            return *this;
        }

        // Destructor
        ~Reverb() {
            //APFs are allocated on heap to persist through calls
            for (NestedAPF<T>* p : this->beforeAPFs) {
                p->~NestedAPF<T>();
                free(p);
            }
            for (NestedAPF<T>* p : this->afterAPFs) {
                p->~NestedAPF<T>();
                free(p);
            }
        }
        
        /**
         * @brief Use this enum type to specify what type of default room you want your reverb sounding like
         * 
         */
        enum class RoomType {
            CUBE, SPHERE, //TODO: Add more/better later
            SQUARE_PYRAMID, CYLINDER,
            CUSTOM
        };

        /**
         * @brief Abstract class to override and provide your own volume and surface area methods:
         * - `getVolume()` return a fixed volume of the same type that `Reverb` is using
         * - `getSurfaceArea()` return a fixed surface area of the same type that `Reverb` is using
         * 
         */
        class CustomRoom {
        public:
            virtual T getVolume() = 0;
            virtual T getSurfaceArea() = 0;
        };


        /**
         * @brief Set the reverb parameters
         * 
         * This is the only way to set the reverb parameters, if you want to just change one you still need to call this function
         * 
         * @param time Time in seconds between successive comb filters (usually keep extremely low and modified rarely)
         * @param regen [0, 1) feedback gain of comb filters to add depth to your sound
         * @param damping [0, 1) feedback gain in LPFs of nested APFs to dampen the high frequencies bouncing back
         * @param roomLength Length parameter in feet of room (affects space according to room shape chosen)
         * @param roomType Preset shape of room for volume/surface area calculations
         * @param absorptionCoefficient [0, 1] How much the walls of the room absorb sound (0 for complete reflection, 1 for complete absorption)
         * 
         * @see giml::Reverb::setRoom()
         */
        void setParams(float time, float regen, float damping, float blend = 0.5f, float roomLength = 1.f, float absorptionCoefficient = 0.75f, RoomType roomType = RoomType::SPHERE) {
            this->setTime(time);
            this->setRegen(regen);
            this->setRoom(roomLength, absorptionCoefficient, roomType);
            this->setDamping(damping);
            this->setBlend(blend);
        }

        void setParams(float time, float regen, float damping, float blend = 0.5f, CustomRoom* customRoom = nullptr) {
            this->setTime(time);
            this->setRegen(regen);
            this->setRoom(customRoom);
            this->setDamping(damping);
            this->setBlend(blend);
        }
        /**
         * @brief Function to process one sound sample through the `Reverb` effect at a time
         * 
         * @param in floating-point type input
         * @return T floating-point (float or double) output
         */
        inline T processSample(const T& in) {

            //this->delayLineInput.writeSample(in);
            if (!(this->enabled)) { return in; }
            T prev = in;
            if (this->numBeforeAPFs > 0) {
                for (auto& apf : this->beforeAPFs) { prev = apf->processSample(prev); }
            }

            // And then the comb filters
            T summedValue = 0;
            for (auto& combFilter : this->parallelCombFilters) {
                summedValue += combFilter.processSample(prev);
            }

            summedValue /= this->numCombFilters; // Need to add this to make sure our signal stays within bounds
            // And then finally insert summedValue into the last set of comb filters
            if (this->numAfterAPFs > 0) {
                for (auto& apf : this->afterAPFs) {
                    summedValue = apf->processSample(summedValue);
                    //prev = apf->processSample(prev);
                }
            }

            return giml::powMix(in, summedValue, this->param__blend);
        }

    private:
        /**
         * @brief Takes the `time` value and calculates the delay indices for all the comb filters and the APFs
         * 
         * @param t time in seconds (you'll want to pass in milliseconds instead to avoid accidental delay effects)
         */
        inline void setTime(float t) { //in sec
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

            //Comb Filter Delay Indices
            float* delayIndices = (float*)calloc(this->numCombFilters, sizeof(float));
            //int delayIndices[this->numCombFilters] = {0};
            delayIndices[0] = this->sampleRate * this->param__time; //They give us max
            delayIndices[this->numCombFilters - 1] = delayIndices[0] / 1.5f; //We know min because of the ratio restriction
            float division = M_PI_4 / (this->numCombFilters - 1); // (pi/4)/number of intermediate comb filters we have left
            for (int i = 1; i < this->numCombFilters - 1; i++) { //Fill in the rest of the comb filters, order does not matter
                delayIndices[i] = delayIndices[0] * (::tanf(i * division) + 2) / 3; //maxDelay * intermediate multiplier
            }
            //Actually set their new delay indices
            for (int i = 0; i < this->numCombFilters; i++) {
                this->parallelCombFilters[i].setDelayIndex(delayIndices[i]);
            }

            free(delayIndices);

            //TODO: Do what we need to do for APF
            int totalAPFs = this->numBeforeAPFs + this->numAfterAPFs;
            if (totalAPFs > 0) { //If we have any APFs to begin with
                delayIndices = (float*)calloc(totalAPFs, sizeof(float));
                delayIndices[0] = (this->sampleRate * this->param__time)/3; //They give us max
                delayIndices[totalAPFs - 1] = delayIndices[0] / 1.5f; //We know min because of the ratio restriction
                float division = M_PI_4 / (totalAPFs - 1); // (pi/4)/number of intermediate comb filters we have left
                for (int i = 1; i < totalAPFs - 1; i++) { //Fill in the rest of the comb filters, order does not matter
                    delayIndices[i] = delayIndices[0] * (::tanf(i * division) + 2) / 3; //maxDelay * intermediate multiplier
                }
                //Actually set their new delay indices
                for (int i = 0; i < this->numBeforeAPFs; i++) {
                    this->beforeAPFs[i]->setDelaySamples(delayIndices[i]);
                }
                for (int i = 0; i < this->numAfterAPFs; i++) {
                    this->afterAPFs[i]->setDelaySamples(delayIndices[this->numBeforeAPFs + i]);
                }

                free(delayIndices);
            }
        }

        /**
         * @brief Takes a feedback gain coefficient and sets the cutoff frequency of the low-pass filters present in the APFs
         * 
         * @param g [0, 1) (non-inclusive because we need gain to be decaying for BIBO stability)
         */
        inline void setDamping(float g) { // [0, 1)
            g = giml::clip<float>(g, 0, 0.97f);
            this->param__damping = g;
            for (auto& apf : this->beforeAPFs) { apf->setLPFFeedbackGain(g); }
            for (auto& apf : this->afterAPFs) { apf->setLPFFeedbackGain(g); }
        }

        /**
         * @brief Set blend 
         * @param b ratio of wet to dry (clamped to [0,1])
         */
        void setBlend(T b) { this->param__blend = giml::clip<T>(b, 0.0, 1.0); }

        /**
         * @brief Takes a feedback gain coefficient and sets the parameter for all the comb filters
         * 
         * @param regen [0, 1) (non-inclusive because we need gain to be decaying for BIBO stability)
         */
        inline void setRegen(float regen) { // [0, 1) (non-inclusive because we need gain to be decaying for BIBO stability)
            regen = giml::clip<float>(regen, 0, 0.999);
            this->param__regen = regen;

            // Recalculate the g value from RT-60 and new damping

            /**
             * @brief Overview of feedback gain calculations
             * lpf_G = g(1-comb_G) where they give us `g`: [0, 1)
             *
             */

             //0 - 1  maps to 0 - sampleRate
            //float cutoffFreq = (1 - regen) * this->sampleRate;

            //Set the LPF feedback gains
            for (int i = 0; i < this->numCombFilters; i++) {
                float g = regen * (1 - ::fabs(this->parallelCombFilters[i].getCombFeedbackGain()));
                this->parallelCombFilters[i].setLPFFeedbackGain(g);
                //this->parallelCombFilters[i].setLPFCutoffFrequency(cutoffFreq);
            }
        }

        /**
         * @brief Takes the aspects of the room and calculates the Comb and APF feedback gains for a proper decay time for the reverb
         * 
         * @param length A either the side or radius of whatever shape you have chosen
         * @param absorptionCoefficient The average absorption of all the collective surfaces in this fake room (0 is completely reflective and 1 is completely absorbive)
         * @param type Pick from any of the default room types
         * @param customRoom If you set CUSTOM in the previous field, you must specify a pointer to your custom room object so that we can properly calculate the proper feedback coefficients
         */
        inline void setRoom(float length, float absorptionCoefficient = 0.75f, RoomType type = RoomType::SPHERE) {
            //Length in feet (ft)
            if (length < 0) { length = 0; }
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
             * Cylinder: V = 1/3 pi r^2 h, SA = 2pi r h + 2pi r^2 (assume h = r though)
             * Square Pyramid: V = 1/3 s^2 h, SA = a^2 + 2a sqrt{a^2/4 + h^2} (assume h = s though)
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
            case RoomType::SQUARE_PYRAMID: {
                //Sand Pyramids absorb a lot more than brick walls, say 0.7-0.9 vs 0.02 for brick
                RT60 = length / (6 * (1 + ::sqrtf(5)) * absorptionCoefficient);
                break;
            }
            case RoomType::CYLINDER: {
                RT60 = length / (8 * absorptionCoefficient);
                break;
            }
            }

            this->calculateAndSetFeedbackCoefficients(RT60);
        }
        /**
         * @brief Call this function if they specify a custom room object instead
         * 
         * @param customRoom You must specify a pointer to your custom room object so that we can properly calculate the proper feedback coefficients
         */
        inline void setRoom(CustomRoom* customRoom = nullptr) {
            float RT60 = customRoom->getVolume() / (2 * customRoom->getSurfaceArea() * customRoom->getAbsorptionCoefficient());
            this->calculateAndSetFeedbackCoefficients(RT60);
        }

        /**
         * Calculating the comb filter feedback gain follows the equation:
         * g = 10^{\frac{3D}{RT60 * sampleFreq}}
         */
        inline void calculateAndSetFeedbackCoefficients(float RT60) {
    
             // Set comb feedback gains corresponding to the newly calculated RT60 decay time
            for (int i = 0; i < this->numCombFilters; i++) {
                float delayIndex = this->parallelCombFilters[i].getDelayIndex();
                float feedbackGain = ::pow(10, -3 * delayIndex / (this->sampleRate * RT60));
                if (feedbackGain > 0.75) { feedbackGain = 0.75; } // TODO: better clamping
                
                // Flip the phase of every other comb filter
                if (i % 2) { this->parallelCombFilters[i].setCombFeedbackGain(-feedbackGain); }
                else { this->parallelCombFilters[i].setCombFeedbackGain(feedbackGain); }
            }

            // Do what we need to do for APF
            for (int i = 0; i < this->numBeforeAPFs; i++) {
                float delayIndex = this->beforeAPFs[i]->getDelaySamples();
                float feedbackGain = ::powf(10, -3 * delayIndex / (this->sampleRate * RT60))/2;
                this->beforeAPFs[i]->setAPFFeedbackGain(-feedbackGain);
            }
            for (int i = 0; i < this->numAfterAPFs; i++) {
                float delayIndex = this->afterAPFs[i]->getDelaySamples();
                float feedbackGain = ::powf(10, -3 * delayIndex / (this->sampleRate * RT60)) / 2;
                this->afterAPFs[i]->setAPFFeedbackGain(-feedbackGain);
            }
        }

        /**
         * @brief An N-th order All-Pass Filter (APF) with functionality to include another N-th order APF in its feedback loop (and continues recursively)
         * 
         * @tparam U 
         */
        template <typename U>
        class NestedAPF { //not the same as 2nd order APF present in Biquad since this is Nth-order
        private:
            CircularBuffer<U> delayLine;
            SinOsc<U> LFO; //TODO: We can try another oscillator?
            NestedAPF<U>* nestedAPF; //Pointer to another nestedAPF inside this one's feedback loop
        
        public:

            // Delete default constructor
            NestedAPF() = delete;

            // Allow NestedAPF to take in a pointer to NestedAPF for placement in the feedback loop of this current APF
            NestedAPF(int sampleRate, NestedAPF<U>* nestedAPF = nullptr) : LFO(sampleRate), nestedAPF(nestedAPF) {
                this->delayLine.allocate(5 * sampleRate);
                //this->LFO.setFrequency(0.1);
            }
            // Copy Constructor
            NestedAPF(const NestedAPF<U>& a) {
                this->delayLine = a.delayLine;
                this->LFO = a.LFO;
                this->lfoDepth = a.lfoDepth;
                this->nestedAPF = a.nestedAPF;

                this->delaySamples = a.delaySamples;
                this->LPFFeedbackGain = a.LPFFeedbackGain;
                this->LPFLast = a.LPFLast;
                this->APFFeedbackGain = a.APFFeedbackGain;
            }

            // Copy assignment operator
            NestedAPF<U>& operator=(const NestedAPF<U>& a) {
                this->delayLine = a.delayLine;
                this->LFO = a.LFO;
                this->lfoDepth = a.lfoDepth;
                this->nestedAPF = a.nestedAPF;

                this->delaySamples = a.delaySamples;
                this->LPFFeedbackGain = a.LPFFeedbackGain;
                this->LPFLast = a.LPFLast;
                this->APFFeedbackGain = a.APFFeedbackGain;

                return *this;
            }

            // Destructor 
            ~NestedAPF() {
                //Deallocate if it hasn't been already
                if (this->nestedAPF) {
                    this->nestedAPF->~NestedAPF();
                    free(this->nestedAPF);
                }
            }

            /**
             * @brief Sets the number of samples the delay starts at. It sets all nested APFs to have 1/4 that delay
             * 
             * @param numSamples  (can be a float for interpolated samples)
             */
            void setDelaySamples(float numSamples) {
                this->delaySamples = numSamples;
                this->nestedAPF->delaySamples = numSamples/4;
            }

            // getter for current delay time
            float getDelaySamples() const { return this->delaySamples; }

            /**
             * @brief Sets the LPF Feedback gain for the embedded LPF(s) in this NestedAPF. Sets all nested ones to 1/4 (recursive)
             * 
             * @param g 
             */
            void setLPFFeedbackGain(float g) {
                this->LPFFeedbackGain = g;
                this->nestedAPF->LPFFeedbackGain = g/4;
            }

            /**
             * @brief Sets the actual APF's Feedback gain. Sets all nested ones to 1/4 (recursive)
             * 
             * @param g 
             */
            void setAPFFeedbackGain(float g) {
                this->APFFeedbackGain = g;
                this->nestedAPF->APFFeedbackGain = g/4;
            }

            U processSample(U in) {

                // Read previous sample from delay line 
                // modulated by oscillator converted to unipolar through *0.5 + 0.5
                U delayedVal = this->delayLine.readSample(this->delaySamples + 
                                (this->LFO.processSample() + 1) / 2 * this->lfoDepth);
                //U delayedVal = this->delayLine.readSample(this->delaySamples);
                //Now go through LPF
                delayedVal = delayedVal * (1 - this->LPFFeedbackGain) + 
                                            this->LPFFeedbackGain * this->LPFLast;
                this->LPFLast = delayedVal; //set next prev to current

                U w = in + this->APFFeedbackGain * delayedVal;

                if (this->nestedAPF) {
                    // Then let's take our value through it
                    w = this->nestedAPF->processSample(w);
                }    
                this->delayLine.writeSample(w); 

                return -this->APFFeedbackGain * w + delayedVal;
            }
        private:
            static const int lfoDepth = 2; // numSamples to go over/under by from original delay of delay line
            float delaySamples = 0.f; // delay in ms converted to how many samples in the past
            T LPFLast = 0;
            float LPFFeedbackGain = 0.f, APFFeedbackGain = 0.f;
        };

        /**
         * @brief Nth order comb filter for use in Reverb
         * @todo write a more general implementation in `filter.hpp`
         */
        template <typename U>
        class CombFilter { //not necessarily a standalone effect in itself
        private:
            CircularBuffer<U> delayLineY;
            U CombFeedbackGain, LPFFeedbackGain;
            float delayIndex;
            bool neg; // Boolean for phase inversion
            Biquad<U> LPF; // TODO: replace with onePole, add DC block
            U LPFLast = 0.f;
            

        public:
            //Constructor
            CombFilter() = delete;
            CombFilter(int sampleRate, 
                        /*const CircularBuffer<U>* pDelayLineIn,*/ 
                        bool negateResponse = false, 
                        float delayIndex = 0, 
                        float combFeedbackGain = 0.f, 
                        float lpfFeedbackGain = 0.f) : 
                        /*pDelayLineX(pDelayLineIn),*/ 
                        neg(negateResponse), 
                        delayIndex(delayIndex), 
                        CombFeedbackGain(combFeedbackGain), 
                        LPFFeedbackGain(lpfFeedbackGain), 
                        LPF(sampleRate) 
            {
                this->delayLineY.allocate(sampleRate * 5);
                this->LPF.setType(Biquad<U>::BiquadUseCase::LPF_1st);
            }

            //Copy constructor
            CombFilter(const CombFilter<U>& c) {
                //this->pDelayLineX = c.pDelayLineX;
                this->delayLineY = c.delayLineY;
                this->delayIndex = c.delayIndex;
                this->CombFeedbackGain = c.CombFeedbackGain;
                this->LPFFeedbackGain = c.LPFFeedbackGain;
                this->neg = c.neg;
                this->LPF = c.LPF;
            }

            // Copy assignment operator
            CombFilter<U>& operator=(const CombFilter<U>& c) {
                //this->pDelayLineX = c.pDelayLineX;
                this->delayLineY = c.delayLineY;
                this->delayIndex = c.delayIndex;
                this->CombFeedbackGain = c.CombFeedbackGain;
                this->LPFFeedbackGain = c.LPFFeedbackGain;
                this->neg = c.neg;
                this->LPF = c.LPF;

                return *this;
            }

            void setDelayIndex(float delayIndex) { this->delayIndex = delayIndex; }
            float getDelayIndex() const { return this->delayIndex; }

            void setCombFeedbackGain(U g) { this->CombFeedbackGain = giml::clip<float>(g, 0.f, 0.999f); }
            U getCombFeedbackGain() const { return this->CombFeedbackGain; }

            void setLPFFeedbackGain(U g) { this->LPFFeedbackGain = giml::clip<float>(g, 0.f, 0.999f); }
            U getLPFFeedbackGain() const { return this->LPFFeedbackGain; }

            // void setLPFCutoffFrequency(float freq) {
            //     this->LPF.setParams(freq);
            // }

            U processSample(U in) {
                // Copied from block diagram

                float yn = this->delayLineY.readSample(delayIndex);
                if (this->neg) { yn = -yn; }

                float filtered = yn * (1 - this->LPFFeedbackGain) +
                    this->LPFFeedbackGain * this->LPFLast;
                this->LPFLast = yn; //set next prev to current



                this->delayLineY.writeSample(in + filtered * this->CombFeedbackGain);
                return yn;

                //float yn = this->delayLineY.readSample(delayIndex);
                //if (this->neg) { yn = -yn; }
                ////float FB = CombFeedbackGain * LPF.processSample(yn);
                //float filtered = yn + (last * this->LPFFeedbackGain);
                ////last = filtered;
                //last = filtered * this->CombFeedbackGain;
                //this->delayLineY.writeSample(in + last);
                //return yn;

                // x[n-D] - g2 x[n-D-1] + g2 y[n-1] + g1 y[n-D]  (g2 = LPF gain)
                // x[n-D] + g2 (y[n-1] - x[n-(D+1)]) + g1 y[n-D]

                //U returnVal = this->pDelayLineX->readSample(delayIndex) // x[n-D]
                //    + LPFFeedbackGain * (this->delayLineY.readSample(1) - this->pDelayLineX->readSample(delayIndex+1)) // g2 (y[n-1] - x[n-(D+1)])
                //    + CombFeedbackGain * this->delayLineY.readSample(delayIndex); // g1 y[n-D]
                //if (this->neg) {
                //    returnVal = -returnVal;
                //}
                //this->delayLineY.writeSample(returnVal);
                //return returnVal;
            }

        };

    };
} // namespace giml

#endif