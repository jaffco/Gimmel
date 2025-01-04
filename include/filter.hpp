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
        T a = 0.0;
        T y_1 = 0.0; 

    public:
        // Default constructor and destructor 
        OnePole() {} 
        ~OnePole() {}

        // Copy constructor
        OnePole(const OnePole<T>& op) {
            this->a = op.a;
            this->y_1 = op.a;
        }

        // Copy assignment operator 
        OnePole<T>& operator=(const OnePole<T>& op) {
            this->a = op.a;
            this->y_1 = op.a;
            return *this;
        }

        /**
         * @brief loPass config: `y_0 = (x_0 * (1-a)) + (y_1 * a)`
         * @param in input sample
         * @return `in * (1-a) + y_1 * a`
         */
        inline T lpf(const T& in) {
            this->y_1 = giml::linMix(in, y_1, a);
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
        void setCutoff(T Hz, T sampleRate) {
            Hz = giml::clip<T>(std::abs(Hz), 0, sampleRate / 2);
            Hz *= -M_2PI / sampleRate;
            this->a = std::pow(M_E, Hz);
        }

        /**
         * @brief set filter coefficient manually
         * @param gVal desired coefficient. 0 = bypass, 1 = sustain
         */
        void setG(T aVal) {
            this->a = giml::clip<T>(aVal, 0, 1);
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
        inline T processSample(const T& in, T g = 0.5) {
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
     * and the various filter types getters for different responses from a singular instance.
     * @todo test, revise for style
     */
    template <typename T>
    class SVF {
    private:
        T q = 0.0, g = 0.0, ff = 0.0, s1fb = 0.0; // filter coefficients
        T x_n = 0.0, hp = 0.0, bp = 0.0, lp = 0.0; // storage for basic outputs
        Trap<T> trap1, trap2; // trap cores containing s1 and s2

    public:
        // Default constructor and destructor 
        SVF() {}
        ~SVF() {}

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

            return *this;
        }

        /**
         * @brief Set filter coefficient by specifying a cutoff frequency, Q factor, and samplerate.
         * See Generating Sound & Organizing Time I - Wakefield and Taylor 2022 Chapter 6 pg. 190
         * @param Hz cutoff frequency in Hz (limited to `sampleRate/4`)
         * @param Q "Quality"
         * @param sampleRate project sample rate
         */
        void setParams(T Hz, T Q, T sampleRate) {
            // frequency warping 
            Hz = giml::clip<T>(std::abs(Hz), 0, sampleRate / 4);
            Hz *= M_PI / sampleRate;
            Hz = std::tan(Hz);

            // set filter coefficients
            if (Q <= 0) { Q = 1e-6; } // clip to avoid division by zero / negative values
            this->q = Q;
            this->g = Hz / (Hz + 1);
            this->s1fb = (1 / this->q) + g;
            this->ff = 1 / (s1fb * g + 1);
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