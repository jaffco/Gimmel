#ifndef GIML_SATURATION_HPP
#define GIML_SATURATION_HPP
#include <math.h>
#include "utility.hpp"
#include "biquad.hpp"
namespace giml {
    /**
     * @brief Waveshaping distortion (BROKEN)
     */
    template <typename T>
    class Saturation : public Effect<T> {
    private:
        Param<T> drive { "drive", 0.001, 50.0, 1.0 };
        Param<T> preAmpGain { "preAmpGain", 0.001, 10.0, 1.0 };
        Param<T> volume { "volume", 0.0, 2.0, 1.0 };
        int sampleRate, oversamplingFactor;
        Biquad<T> antiAliasingFilter;
        T prevX = 0;

    public:
        Saturation(int sampleRate, int oversamplingFactor = 1) : sampleRate(sampleRate), oversamplingFactor(oversamplingFactor), antiAliasingFilter(Biquad<T>{sampleRate})  {
            this->registerParameters(drive, preAmpGain, volume);
            this->antiAliasingFilter.setType(Biquad<T>::BiquadUseCase::LPF_2nd);
            this->antiAliasingFilter.setParams(this->sampleRate * oversamplingFactor / 2); //TODO: Verify this cutoff frequency
        }
        ~Saturation() {}
        //Copy constructor
        Saturation(const Saturation& s) : Effect<T>(s) {
            this->sampleRate = s.sampleRate;
            this->oversamplingFactor = s.oversamplingFactor;
            this->drive = s.drive;
            this->volume = s.volume;
            this->preAmpGain = s.preAmpGain;
            this->antiAliasingFilter = s.antiAliasingFilter;
            this->prevX = s.prevX;
        }
        //Copy assignment constructor
        Saturation& operator=(const Saturation& s) {
            Effect<T>::operator=(s);
            this->sampleRate = s.sampleRate;
            this->oversamplingFactor = s.oversamplingFactor;
            this->drive = s.drive;
            this->volume = s.volume;
            this->preAmpGain = s.preAmpGain;
            this->antiAliasingFilter = s.antiAliasingFilter;
            this->prevX = s.prevX;
            
            return *this;
        }
        
        inline T processSample(const T& in) override {
            if (!(this->enabled)) {
                return in;
            }
            
            T returnVal = in * this->preAmpGain();
            if (this->oversamplingFactor > 1) {
                //TODO: Then we need to oversample, apply process

                /*
                1. Linear interpolation of factor-1 points in between previous sample and current sample (include current sample)
                2. apply tan to all samples
                3. anti-aliasing filter
                4. decimate and return
                */

                T* arrValues = (T*) malloc(this->oversamplingFactor * sizeof(T));
                //arrValues[this->oversamplingFactor - 1] = in; //Set last value in array to current input

                T delta = (in - prevX) / this->oversamplingFactor;

                

                for (int i = 0; i < this->oversamplingFactor; i++) {
                    arrValues[i] = in * this->preAmpGain() + i * delta; //Linear interpolation for each sample (1st is previous real sample and last is current input)
                    //TODO: Apply correct distortion function here for each sample
                    //arrValues[i] = ::tanhf(this->drive * arrValues[i]) / ::tanhf(this->drive);

                    // asymmetrical distortion with 
                    if (arrValues[i] >= 0) { // if x positive 
                        arrValues[i] = tanhf(this->drive() * arrValues[i]) / tanhf(this->drive());
                    }
                    else { // if x negative 
                        arrValues[i] = tanhf(3*this->drive() * arrValues[i]) / tanhf(3*this->drive());
                    }


                    arrValues[i] = this->antiAliasingFilter.processSample(arrValues[i]); //TODO: See if anti-aliasing LPF is actually needed
                }
                returnVal = 0;
                //returnVal = arrValues[this->oversamplingFactor-1];
                for (int i = 0; i < this->oversamplingFactor; i++) {
                    //TODO: does averaging the values actually act as another low-pass filter?
                    returnVal += arrValues[i];
                }
                returnVal /= this->oversamplingFactor;

            }
            else {
                // symmetrical distortion with tanh
                // returnVal = ::tanhf(this->drive * in) / ::tanhf(this->drive);

                // asymmetrical distortion with 
                 if (in >= 0) { // if x positive 
                     returnVal = ::tanhf(this->drive() * in) / ::tanhf(this->drive());
                 }
                 else { // if y negative 
                     returnVal = ::tanhf(3*this->drive() * in) / ::tanhf(3*this->drive());
                 }

                // oversampling ?
            }
            
            
            prevX = in;
            return returnVal * this->volume();
        }

        void setVolume(float v) {
            this->volume = dBtoA(v);
        }

        void setDrive(float d) {
            if (d <= 0.f) {
                d += 1e-6;
                printf("Drive set to pseudo-zero value, supply a positive float/n");
            }
            this->drive = dBtoA(d);
        }

        void setPreAmpGain(float g) {
            if (g == 0) {
                g += 1e-6;
            }
            this->preAmpGain = dBtoA(g);
        }
    };
}
#endif