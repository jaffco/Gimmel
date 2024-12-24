#ifndef GIML_BIQUAD_HPP
#define GIML_BIQUAD_HPP
#include <math.h>
#include "utility.hpp"
namespace giml {
    template <typename T>
    class Biquad : public Effect<T> {
    public:
        enum class BiquadUseCase {
            PassThroughDefault, //Default type until parameters are set
            
            //Basic RC
            LPF_1st,        // First-Order Lowpass Filter (LPF)
            HPF_1st,        // First-Order Highpass Filter (HPF)
            
            //Second-order filters
            LPF_2nd,        // Second-Order LPF
            HPF_2nd,        // Second-Order HPF
            BPF,            // Bandpass Filter (BPF)
            BSF,            // Bandstop Filter (BSF)
            
            //Butterworth Filters
            LPF_Butterworth,// Butterworth LPF
            HPF_Butterworth,// Butterworth HPF
            BPF_Butterworth,// Butterworth BPF
            BSF_Butterworth,// Butterworth BSF

            //Linkwitz-Riley - steeper than Butterworth
            LPF_LR,         // Linkwitz-Riley LPF
            HPF_LR,         // Linkwitz-Riley HPF

            //All-pass filters (no frequency changes, only phase shift)
            APF_1st,        // First-Order Allpass Filter (APF)
            APF_2nd,        // Second-Order APF -> 2nd-Order APF has double the phase shift

            //Shelf filters
            LSF,        // First-Order Low Shelf Filter (LSF)
            HSF,        // First-Order High Shelf Filter (HSF)

            //Parametric EQ Filters
            PEQ,            //(non-const Q)
            PEQ_constQ      // Parametric EQ Filter (const Q)
        };

        //Constructor
        Biquad() = delete;
        Biquad(int sampleRate) : sampleRate(sampleRate) {}
        Biquad(const Biquad<T>& b) {
            this->useCase = b.useCase;

            this->sampleRate = b.sampleRate;

            this->a0 = b.a0;
            this->a1 = b.a1;
            this->a2 = b.a2;

            this->b1 = b.b1;
            this->b2 = b.b2;

            this->prevX1 = b.prevX1;
            this->prevX2 = b.prevX2;
            this->prevY1 = b.prevY1;
            this->prevY2 = b.prevY2;

            this->cutoffFrequency = b.cutoffFrequency;
            this->Q = b.Q;
            this->gainDB = b.gainDB;
        }
        // Copy assignment operator
        Biquad<T>& operator=(const Biquad<T>& b) {
            this->useCase = b.useCase;

            this->sampleRate = b.sampleRate;

            this->a0 = b.a0;
            this->a1 = b.a1;
            this->a2 = b.a2;

            this->b1 = b.b1;
            this->b2 = b.b2;

            this->prevX1 = b.prevX1;
            this->prevX2 = b.prevX2;
            this->prevY1 = b.prevY1;
            this->prevY2 = b.prevY2;

            this->cutoffFrequency = b.cutoffFrequency;
            this->Q = b.Q;
            this->gainDB = b.gainDB;

            return *this;
        }
        //TODO: Copy constructor + Copy assignment constructor
        ~Biquad() {}

        void setType(BiquadUseCase type) {
            this->useCase = type;
            this->setParams(this->cutoffFrequency, this->Q, this->gainDB); //Recalculate coefficients
        }

        BiquadUseCase getType() const {
            return this->useCase;
        }

        void setParams(float cutoffFrequency, float Q = 0.707, float gainDB = 0.f) {
            this->cutoffFrequency = cutoffFrequency;
            this->Q = Q;
            this->gainDB = gainDB;

            switch (useCase) {
            case BiquadUseCase::PassThroughDefault:
                printf("Make sure you set filter type first before you set parameters/n");
                break;
            case BiquadUseCase::LPF_1st:
                this->setParams__LPF_1st(cutoffFrequency);
                break;
            case BiquadUseCase::HPF_1st:
                this->setParams__HPF_1st(cutoffFrequency);
                break;
            case BiquadUseCase::APF_1st:
                this->setParams__APF_1st(cutoffFrequency);
                break;
            case BiquadUseCase::LPF_2nd:
                this->setParams__LPF_2nd(cutoffFrequency, Q);
                break;
            case BiquadUseCase::HPF_2nd:
                this->setParams__HPF_2nd(cutoffFrequency, Q);
                break;
            case BiquadUseCase::BSF:
                this->setParams__BSF(cutoffFrequency, Q);
                break;
            case BiquadUseCase::LPF_Butterworth:
                this->setParams__LPF_Butterworth(cutoffFrequency);
                break;
            case BiquadUseCase::HPF_Butterworth:
                this->setParams__HPF_Butterworth(cutoffFrequency);
                break;
            case BiquadUseCase::BSF_Butterworth:
                this->setParams__BSF_Butterworth(cutoffFrequency, Q);
                break;
            case BiquadUseCase::APF_2nd:
                this->setParams__APF_2nd(cutoffFrequency, Q);
                break;
            case BiquadUseCase::PEQ_constQ:
                this->setParams__PEQ_constQ(cutoffFrequency, Q, gainDB);
                break;
            case BiquadUseCase::BPF:
                this->setParams__BPF(cutoffFrequency, Q);
                break;
            case BiquadUseCase::BPF_Butterworth:
                this->setParams__BPF_Butterworth(cutoffFrequency, Q);
                break;
            case BiquadUseCase::LSF:
                this->setParams__LSF(cutoffFrequency, Q, gainDB);
                break;
            case BiquadUseCase::HSF:
                this->setParams__HSF(cutoffFrequency, Q, gainDB);
                break;
            }
        }

        inline T processSample(const T& in) {
            T returnVal = {0};
            switch (useCase) {
            case BiquadUseCase::PassThroughDefault:
                returnVal = in;
                printf("Make sure you set filter type first before you use the Biquad struct/n");
                break;
            case BiquadUseCase::LPF_1st:
            case BiquadUseCase::HPF_1st:
            case BiquadUseCase::APF_1st:
                returnVal = a0 * in + a1 * prevX1 - b1 * prevY1;
                break;
            case BiquadUseCase::LPF_2nd:
            case BiquadUseCase::HPF_2nd:
            case BiquadUseCase::LPF_Butterworth:
            case BiquadUseCase::HPF_Butterworth:
            case BiquadUseCase::APF_2nd:
            case BiquadUseCase::LSF:
            case BiquadUseCase::HSF:
            case BiquadUseCase::PEQ_constQ:
                returnVal = a0 * in + a1 * prevX1 + a2*prevX2 - b1 * prevY1 - b2*prevY2;
                break;
            /*case BiquadUseCase::BPF:
            case BiquadUseCase::BPF_Butterworth:
                returnVal = a0 * in + a2 * prevX2 - b1 * prevY1 - b2 * prevY2;
                break;*/
            

            //TODO: Not yet implemented
            case BiquadUseCase::LPF_LR:
            case BiquadUseCase::HPF_LR:
            case BiquadUseCase::PEQ:
            case BiquadUseCase::BSF:
            case BiquadUseCase::BSF_Butterworth:
            case BiquadUseCase::BPF:
            case BiquadUseCase::BPF_Butterworth:
                printf("Not yet implemented!!/n");
                returnVal = 0;
                break;
            default:
                printf("How did we get here!!/n");
                returnVal = 0;
            }

            //Back propagate inputs and outputs
            prevX2 = prevX1;
            prevX1 = in;

            prevY2 = prevY1;
            prevY1 = returnVal;

            if (!(this->enabled)) {
                return in;
            }

            return returnVal;
        }
    private:
        BiquadUseCase useCase = BiquadUseCase::PassThroughDefault;

        int sampleRate;

        T a0=1, a1=0, a2=0,   //Numeratror coefficients (set a0 to 1 for default passthrough)
            b1=0, b2=0;     //Denominator coefficients
        //Past 2 x,y values
        T prevX1 = 0, prevX2 = 0,
            prevY1 = 0, prevY2 = 0;

        float cutoffFrequency = 1000.f, Q = 0.707f, gainDB = 0.f;

        void setParams__LPF_1st(float cutoffFrequency) {
            //Set type to low-pass if not already
            if (this->useCase != BiquadUseCase::LPF_1st) {
                this->useCase = BiquadUseCase::LPF_1st;
            }
            float cutoffAngle = M_2PI * cutoffFrequency / this->sampleRate;
            float gamma = ::cosf(cutoffAngle) / (1 + ::sinf(cutoffAngle));
            this->a0 = (1 - gamma) / 2;
            this->a1 = this->a0;
            this->a2 = 0;
            this->b1 = -gamma;
            this->b2 = 0;
        }

        void setParams__HPF_1st(float cutoffFrequency) {
            //Set type to low-pass if not already
            if (this->useCase != BiquadUseCase::HPF_1st) {
                this->useCase = BiquadUseCase::HPF_1st;
            }
            float cutoffAngle = M_2PI * cutoffFrequency / this->sampleRate;
            float gamma = ::cosf(cutoffAngle) / (1 + ::sinf(cutoffAngle));
            this->a0 = (1 + gamma) / 2;
            this->a1 = -this->a0;
            this->a2 = 0;
            this->b1 = -gamma;
            this->b2 = 0;
        }

        void setParams__LPF_2nd(float cutoffFrequency, float Q) {
            //Set type to low-pass if not already
            if (this->useCase != BiquadUseCase::LPF_2nd) {
                this->useCase = BiquadUseCase::LPF_2nd;
            }
            float cutoffAngle = M_2PI * cutoffFrequency / this->sampleRate;
            float d = ::sinf(cutoffAngle) / (2 * Q);
            float Beta = (1 + (1 - d) / (1 + d)) / 2;
            float gamma = Beta * ::cosf(cutoffAngle);


            this->a0 = (Beta - gamma) / 2;
            this->a1 = Beta - gamma;
            this->a2 = this->a0;
            this->b1 = -2 * gamma;
            this->b2 = 2 * Beta - 1;
        }

        void setParams__HPF_2nd(float cutoffFrequency, float Q) {
            //Set type to high-pass if not already
            if (this->useCase != BiquadUseCase::HPF_2nd) {
                this->useCase = BiquadUseCase::HPF_2nd;
            }
            float cutoffAngle = M_2PI * cutoffFrequency / this->sampleRate;
            float d = ::sinf(cutoffAngle) / (2 * Q);
            float Beta = (1 + (1 - d) / (1 + d)) / 2;
            float gamma = Beta * ::cosf(cutoffAngle);


            this->a0 = (Beta + gamma) / 2;
            this->a1 = -(Beta + gamma);
            this->a2 = this->a0;
            this->b1 = -2 * gamma;
            this->b2 = 2 * Beta - 1;
        }

        void setParams__BPF(float cutoffFrequency, float Q) {
            //Set type to band-pass if not already
            if (this->useCase != BiquadUseCase::BPF) {
                this->useCase = BiquadUseCase::BPF;
            }
            float K = ::tanf(M_PI * cutoffFrequency / this->sampleRate);
            float KSquared = K * K;
            float delta = KSquared * Q + K + Q;

            this->a0 = K / delta;
            this->a1 = 0;
            this->a2 = -this->a0;

            this->b1 = 2 * Q * (KSquared - 1) / delta;
            this->b2 = (KSquared - K + Q) / delta;
        }

        void setParams__BSF(float cutoffFrequency, float Q) {
            //Set type to band-stop if not already
            if (this->useCase != BiquadUseCase::BSF) {
                this->useCase = BiquadUseCase::BSF;
            }
            float K = ::tanf(M_PI * cutoffFrequency / this->sampleRate);
            float KSquared = K * K;
            float delta = KSquared * Q + K + Q;

            this->a0 = Q * (KSquared + 1) / delta;
            this->a1 = 2 * Q * (KSquared - 1) / delta;
            this->a2 = this->a0;

            this->b1 = this->a1;
            this->b2 = (KSquared * Q - K + Q) / delta;
        }

        void setParams__LPF_Butterworth(float cutoffFrequency) {
            //Set type to low-pass if not already
            if (this->useCase != BiquadUseCase::LPF_Butterworth) {
                this->useCase = BiquadUseCase::LPF_Butterworth;
            }
            //Q is fixed to sqrt(2) to avoid resonance
            float C = 1 / ::tanf(M_PI * cutoffFrequency / this->sampleRate);
            float CSquared = C * C;

            this->a0 = 1 / (1 + M_SQRT2 * C + CSquared);
            this->a1 = 2 * this->a0;
            this->a2 = this->a0;

            this->b1 = 2 * this->a0 * (1 - CSquared);
            this->b2 = this->a0 * (1 - M_SQRT2 * C + CSquared);
        }

        void setParams__HPF_Butterworth(float cutoffFrequency) {
            //Set type to high-pass if not already
            if (this->useCase != BiquadUseCase::HPF_Butterworth) {
                this->useCase = BiquadUseCase::HPF_Butterworth;
            }
            //Q is fixed to sqrt(2) to avoid resonance
            float C = ::tanf(M_PI * cutoffFrequency / this->sampleRate);
            float CSquared = C * C;

            this->a0 = 1 / (1 + M_SQRT2 * C + CSquared);
            this->a1 = -2 * this->a0;
            this->a2 = this->a0;

            this->b1 = 2 * this->a0 * (CSquared - 1);
            this->b2 = this->a0 * (1 - M_SQRT2 * C + CSquared);
        }

        void setParams__BPF_Butterworth(float cutoffFrequency, float Q) {
            //Set type to band-pass if not already
            if (this->useCase != BiquadUseCase::BPF_Butterworth) {
                this->useCase = BiquadUseCase::BPF_Butterworth;
            }
            float BW = cutoffFrequency / Q; //Bandwidth
            float C = 1 / ::tanf(M_PI * cutoffFrequency * BW / this->sampleRate);
            float D = 2 * ::tanf(M_2PI * cutoffFrequency / this->sampleRate);

            this->a0 = 1 / (1 + C);
            this->a1 = 0;
            this->a2 = -this->a0;

            this->b1 = -this->a0 * C * D;
            this->b2 = this->a0 * (C - 1);
        }

        void setParams__BSF_Butterworth(float cutoffFrequency, float Q) {
            //Set type to band-stop if not already
            if (this->useCase != BiquadUseCase::BSF_Butterworth) {
                this->useCase = BiquadUseCase::BSF_Butterworth;
            }
            float BW = cutoffFrequency / Q; //Bandwidth
            float C = ::tanf(M_PI * cutoffFrequency * BW / this->sampleRate);
            float D = 2 * ::tanf(M_2PI * cutoffFrequency / this->sampleRate);

            this->a0 = 1 / (1 + C);
            this->a1 = 0;
            this->a2 = -this->a0;

            this->b1 = -this->a0 * C * D;
            this->b2 = this->a0 * (C - 1);
        }

        //void setParams__LPF_LR(float cutoffFrequency) {
        //    //Set type to low-pass if not already
        //    if (this->useCase != BiquadUseCase::BSF_Butterworth) {
        //        this->useCase = BiquadUseCase::BSF_Butterworth;
        //    }
        //    float BW = cutoffFrequency / Q; //Bandwidth
        //    float C = ::tanf(M_PI * cutoffFrequency * BW / this->sampleRate);
        //    float D = 2 * ::tanf(M_2PI * cutoffFrequency / this->sampleRate);

        //    this->a0 = 1 / (1 + C);
        //    this->a1 = 0;
        //    this->a2 = -this->a0;

        //    this->b1 = -this->a0 * C * D;
        //    this->b2 = this->a0 * (C - 1);
        //}


        void setParams__APF_1st(float cutoffFrequency) {
            //Set type to all-pass if not already
            if (this->useCase != BiquadUseCase::APF_1st) {
                this->useCase = BiquadUseCase::APF_1st;
            }
            float t = ::tanf(M_PI * cutoffFrequency / this->sampleRate);
            float alpha = (t - 1) / (t + 1);
            this->a0 = alpha;
            this->a1 = 1;
            this->a2 = 0;

            this->b1 = alpha;
            this->b2 = 0;
        }

        void setParams__APF_2nd(float cutoffFrequency, float Q) {
            //Set type to all-pass if not already
            if (this->useCase != BiquadUseCase::APF_2nd) {
                this->useCase = BiquadUseCase::APF_2nd;
            }
            float cutoffAngle = M_2PI * cutoffFrequency / this->sampleRate;
            float alpha = ::sinf(cutoffAngle) / (2 * Q);

            this->a0 = (1 - alpha) / (1 + alpha);
            this->a1 = -2 * ::cosf(cutoffAngle) / (1 + alpha);
            this->a2 = 1;

            this->b1 = this->a1;
            this->b2 = this->a0;

            /*float BW = cutoffFrequency / Q;
            float t = ::tanf(M_PI * BW / this->sampleRate);
            float alpha = (t - 1) / (t + 1);
            float Beta = -::cosf(M_2PI * cutoffFrequency / this->sampleRate);
            this->a0 = -alpha;
            this->a1 = Beta * (1 - alpha);
            this->a2 = 1;

            this->b1 = Beta * (1 - alpha);
            this->b2 = -alpha;*/
        }

        void setParams__LSF(float cutoffFrequency, float Q, float gainDB) {
            //Set type to low-shelf if not already
            if (this->useCase != BiquadUseCase::LSF) {
                this->useCase = BiquadUseCase::LSF;
            }
            float cutoffAngle = M_2PI * cutoffFrequency / this->sampleRate;
            float A = giml::dBtoA(gainDB);


            float cosss = ::cosf(cutoffAngle);
            //Conversion between Q and shelf steepness (S)
            //float gamma = ::sinf(cutoffAngle) * ::sqrtf((A * A + 1) * ((1 / (Q * Q) - 2) / (A + 1 / A)) + 2 * A);
            float gamma = ::sinf(cutoffAngle) * ::sqrtf(A) / Q;
            float alpha = (A + 1) * cosss;
            float beta = (A - 1) * cosss;
            float c = (A + 1) + beta + gamma;

            this->a0 = A * (A + 1 - beta + gamma) / c;
            this->a1 = 2 * A * (A - 1 - alpha) / c;
            this->a2 = A * (A + 1 - beta - gamma) / c;

            this->b1 = -2 * (A - 1 + alpha) / c;
            this->b2 = (A + 1 + beta - gamma) / c;


            /*float delta = 4 * ::tanf(cutoffFrequency / 2) / (1 + A);
            float gamma = (1 - delta) / (1 + delta);

            this->a0 = (1 - gamma) / 2;
            this->a1 = this->a0;
            this->a2 = 0;

            this->b1 = -gamma;
            this->b2 = 0;

            this->dry = 1;
            this->wet = A - 1;*/
        }

        void setParams__HSF(float cutoffFrequency, float Q, float gainDB) {
            //Set type to high-shelf if not already
            if (this->useCase != BiquadUseCase::HSF) {
                this->useCase = BiquadUseCase::HSF;
            }
            float cutoffAngle = M_2PI * cutoffFrequency / this->sampleRate;
            float A = giml::dBtoA(gainDB);

            float cosss = ::cosf(cutoffAngle);
            //Conversion between Q and shelf steepness (S)
            //float gamma = ::sinf(cutoffAngle) * ::sqrtf((A * A + 1) * ((1 / (Q * Q) - 2) / (A + 1 / A)) + 2 * A);
            float gamma = ::sinf(cutoffAngle) * ::sqrtf(A) / Q;
            float alpha = (A + 1) * cosss;
            float beta = (A - 1) * cosss;
            float c = (A + 1) - beta + gamma;

            this->a0 = A * (A + 1 + beta + gamma) / c;
            this->a1 = -2 * A * (A - 1 + alpha) / c;
            this->a2 = A * (A + 1 + beta - gamma) / c;

            this->b1 = 2 * (A - 1 - alpha) / c;
            this->b2 = (A + 1 - beta - gamma) / c;

            //float delta = (1 + multiplier) * ::tanf(cutoffFrequency / 2) / 4;
            //float gamma = (1 - delta) / (1 + delta);

            //this->a0 = (1 + gamma) / 2;
            //this->a1 = -this->a0;
            //this->a2 = 0;

            //this->b1 = -gamma;
            //this->b2 = 0;

            //this->dry = 1;
            //this->wet = multiplier - 1;
        }

        void setParams__PEQ_constQ(float centerFrequency, float Q, float gainDB) {
            //Set type to parametric EQ (const Q behavior) if not already
            if (this->useCase != BiquadUseCase::PEQ_constQ) {
                this->useCase = BiquadUseCase::PEQ_constQ;
            }

            float K = ::tanf(M_PI * centerFrequency / this->sampleRate);
            float KSquared = K * K;
            float vol = giml::dBtoA(gainDB);

            float d = 1 + K / Q + KSquared;
            float e = 1 + K / (vol * Q) + KSquared;

            float alpha = 1 + vol * K / Q + KSquared;
            float Beta = 2 * (KSquared - 1);
            float gamma = 1 - vol * K / Q + KSquared;
            float delta = 1 - K / Q + KSquared;
            float nu = 1 - K / (vol * Q) + KSquared;

            if (gainDB >= 0) {
                //Then we are boosting this range
                this->a0 = alpha / d;
                this->a1 = Beta / d;
                this->a2 = gamma / d;

                this->b1 = this->a1;
                this->b2 = delta / d;
            }
            else {
                //We are cutting this range
                this->a0 = d / e;
                this->a1 = Beta / e;
                this->a2 = delta / e;

                this->b1 = Beta / e;
                this->b2 = nu / e;
            }
        }


    };
}

#endif