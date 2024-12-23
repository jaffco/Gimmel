#ifndef GIML_FILTER_HPP
#define GIML_FILTER_HPP
namespace giml {
    /**
     * @brief implements a simple one-pole filter
     * TODO: add constructors
     */
    template <typename T>
    class onePole {
    protected:
        T a = 0;
        T y_1 = 0; 

    public:
        /**
         * @brief loPass config: `y_0 = (x_0 * (1-a)) + (y_1 * a)`
         * @param in input sample
         * @return `in * (1-a) + y_1 * a`
         */
        T lpf(T in) {
          this->y_1 = giml::linMix(in, y_1, a);
          return y_1;
        }

        /**
         * @brief hiPass config: `y_0 = x_0 - lpf(x_0)`
         * @param in input sample
         * @return `in - lpf(in)`
         */
        T hpf(T in) {
          this->lpf(in);
          return in - this->y_1;
        }

        /**
         * @brief Set filter coefficient by specifying a cutoff frequency and sample rate.
         * See Generating Sound & Organizing Time I - Wakefield and Taylor 2022 Chapter 6 pg. 166
         * @param Hz cutoff frequency in Hz
         * @param sampleRate project sample rate
         */
        void setCutoff(T Hz, T sampleRate) {
          Hz = giml::clip<T>(::abs(Hz), 0, sampleRate / 2);
          Hz *= -M_2PI / sampleRate;
          this->a = ::pow(M_E, Hz);
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