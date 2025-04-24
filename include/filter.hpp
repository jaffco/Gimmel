#ifndef GIML_FILTER_HPP
#define GIML_FILTER_HPP
#include "utility.hpp"
namespace giml {
    /**
     * @brief implements a simple one-pole filter
     */
    template <typename T>
    class OnePole {
    private:
        T g = 0.0;
        T y_1 = 0.0; 

    public:
        // Default constructor and destructor 
        OnePole() {} 
        ~OnePole() {}

        // Copy constructor
        OnePole(const OnePole<T>& op) {
            this->g = op.g;
            this->y_1 = op.y_1;
        }

        // Copy assignment operator 
        OnePole<T>& operator=(const OnePole<T>& op) {
            this->g = op.g;
            this->y_1 = op.y_1;
            return *this;
        }

        /**
         * @brief loPass config: `y_0 = (x_0 * (1-a)) + (y_1 * a)`
         * @param in input sample
         * @return `in * (1-a) + y_1 * a`
         */
        inline T lpf(const T& in) {
            this->y_1 = giml::linMix(in, y_1, g);
            return y_1;
        }

        /**
         * @brief hiPass config: `y_0 = x_0 - lpf(x_0)`
         * @param in input sample
         * @return `in - lpf(in)`
         */
        inline T hpf(const T& in) {
            return in - this->lpf(in);
        }

        /**
         * @brief Set filter coefficient by specifying a cutoff frequency and sample rate.
         * See Generating Sound & Organizing Time I - Wakefield and Taylor 2022 Chapter 6 pg. 166
         * @param Hz cutoff frequency in Hz
         * @param sampleRate project sample rate
         */
        void setCutoff(const T& Hz, const T& sampleRate) {
            T freq = giml::clip<T>(::abs(Hz), 0, sampleRate / 2);
            freq *= -M_2PI / sampleRate;
            this->g = ::pow(M_E, freq);
        }

        /**
         * @brief set filter coefficient manually
         * @param gVal desired coefficient. 0 = bypass, 1 = sustain
         */
        void setG(const T& gVal) {
            this->g = giml::clip<T>(gVal, 0, 1);
        }

    };

    /**
     * Trapezoidal (BLT) integrator core
     * See Generating Sound & Organizing Time I - Wakefield and Taylor 2022 Chapter 6 pg. 184
     */
    template <typename T>
    class Trap {
    private:
        T hist = 0.0;

    public:
        // Default constructor and destructor 
        Trap() {}
        ~Trap() {}

        // Copy constructor
        Trap(const Trap<T>& t) {
            this->hist = t.hist;
        }

        // Copy assignment operator 
        Trap<T>& operator=(const Trap<T>& t) {
            this->hist = t.hist;
            return *this;
        }

        /**
         * @brief Updates state and returns `y[0]`
         * @param in input sample
         * @return integrator output
         */
        inline T processSample(const T& in, const T& g = 0.5) {
            T split = in * g;
            T y_0 = split + hist;
            hist = y_0 + split;
            return y_0;
        }

        /**
         * @brief Getter for `hist`
         * @return value of hist
         */
        inline T state() const {
            return this->hist;
        }

    };

    /**
     * @brief State Variable Filter. Once constructed, 
     * call the operator with an input once per sample to update state,
     * and the various filter type getters for different responses from a singular instance.
     */
    template <typename T>
    class SVF {
    private:
        T q = 0.0, g = 0.0, ff = 0.0, s1fb = 0.0; // filter coefficients
        T x_n = 0.0, hp = 0.0, bp = 0.0, lp = 0.0; // storage for basic outputs
        Trap<T> trap1, trap2; // trap cores containing s1 and s2
        T freqFactor = 0.0;

    public:
        // Default constructor and destructor 
        SVF() {}
        ~SVF() {}

        SVF(T sampleRate) {
            this->freqFactor = T(M_PI) / sampleRate;
        }

        // Copy constructor
        SVF(const SVF<T>& s) {
            this->q = s.q;
            this->g = s.g;
            this->ff = s.ff;
            this->s1fb = s.s1fb;

            this->x_n = s.x_n;
            this->hp = s.hp;
            this->bp = s.bp;
            this->lp = s.lp;

            this->trap1 = s.trap1;
            this->trap2 = s.trap2;

            this->freqFactor = s.freqFactor;
        }

        // Copy assignment operator 
        SVF<T>& operator=(const SVF<T>& s) {
            this->q = s.q;
            this->g = s.g;
            this->ff = s.ff;
            this->s1fb = s.s1fb;

            this->x_n = s.x_n;
            this->hp = s.hp;
            this->bp = s.bp;
            this->lp = s.lp;
            
            this->trap1 = s.trap1;
            this->trap2 = s.trap2;

            this->freqFactor = s.freqFactor;

            return *this;
        }

        /**
         * @brief Set filter coefficient by specifying a cutoff frequency, Q factor, and samplerate.
         * See Generating Sound & Organizing Time I - Wakefield and Taylor 2022 Chapter 6 pg. 190
         * @param Hz cutoff frequency in Hz (limited to `sampleRate/4`)
         * @param Q "Quality"
         * @param sampleRate project sample rate
         */
        inline void setParams(const T& Hz, const T& Q, const T& sampleRate) {
            // frequency warping 
            T freq = giml::clip<T>(::abs(Hz), 0, sampleRate / 4);
            freq *= freqFactor;
            freq = tan(freq);

            // set filter coefficients
            this->q = std::max(Q, T(1e-6)); // avoid div by zero / negative values
            this->g = freq / (freq + T(1.0));
            this->s1fb = (T(1.0) / this->q) + this->g;
            this->ff = T(1.0) / (this->s1fb * this->g + T(1.0));
        }

        /**
         * @brief Updates state, no return. Call once per sample, 
         * and use `loPass()`, `hiPass()` etc. to get different filter types.
         */
        inline void operator()(const T& in) {
            this->x_n = in;
            this->hp = ff * (in - trap2.state() - s1fb * trap1.state());
            bp = trap1.processSample(hp, g);
            lp = trap2.processSample(bp, g);
        }

        inline T loPass() const { return this->lp; }
        inline T hiPass() const { return this->hp; }
        inline T bandPass() const { return this->bp; }
        inline T peak() const { return lp - hp; }
        inline T UBP() const { return bp / q; } // what makes this bp different?
        inline T bshelf() const { return x_n + bp / q; }
        inline T notch() const { return x_n - bp / q; }
        inline T allPass() const { return x_n - 2 * (bp / q); }

    };

} // namespace giml
#endif