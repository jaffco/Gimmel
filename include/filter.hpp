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

} // namespace giml
#endif