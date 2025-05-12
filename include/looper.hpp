#ifndef GIML_PHASER_HPP
#define GIML_PHASER_HPP
#include "utility.hpp"

namespace giml {


    template <typename T>
    class Looper : public Effect<T> {
        private:
            int sampleRate;
            float blend;
            giml::CircularBuffer<T> buffer; // circular buffer to store the looped track
            bool isRecording = false;
            bool loopPlaybackActive = false;
            size_t loopLength = 0;
            size_t loopPlaybackCursor = 0;

        public:
            // Constructor
            Looper() = delete;
            Looper(int sampleRate, float maxSizeMillis = 60000) : sampleRate(sampleRate) { // max loop time is 60 sec by default
                // this->setParams();
                this->buffer.allocate(giml::millisToSamples(maxSizeMillis, sampleRate));
            }
            
            // Copy constructor
            Looper(const Looper<T>& l) {
                this->sampleRate = l.sampleRate;
                this->blend = l.blend;
                this->buffer = l.buffer;
                this->isRecording = l.isRecording;
                this->loopPlaybackActive = l.loopPlaybackActive;
                this->loopLength = l.loopLength;
                this->loopPlaybackCursor = l.loopPlaybackCursor;
            }

            //Copy assignment operator
            Looper<T>& operator=(const Looper<T>& l) {
                this->sampleRate = l.sampleRate;
                this->blend = l.blend;
                this->buffer = l.buffer;
                this->isRecording = l.isRecording;
                this->loopPlaybackActive = l.loopPlaybackActive;
                this->loopLength = l.loopLength;
                this->loopPlaybackCursor = l.loopPlaybackCursor;

                return *this;
            }
            
            // Destructor
            ~Looper() {}

            /**
             * @brief 
             * 
             * 
             * Let's outline the UI behavior of this looper
             * 
             * Press and Hold to record
             * when recording, output exactly what's being written to the circular buffer
             * 
             * Release to stop recording and start the playback
             * - store last index of where the looping needs to end
             * 
             * 
             * When playing back, the switch toggles whether we hear the looped signal + the input or just the input
             * 
             * IF we are recording while enabled, let us += in the circular buffer to loop over a looped track
             * 
             * IF we are recording while disabled, let us just = in the circular buffer (overwrite) 
             * 
             * 
             * 
             * @param in 
             * @return T 
             */
            inline T processSample(const T& in) {
                T returnVal;
                if (!(this->enabled)) {
                    // We want to just return through-put
                    
                    // But if we are in the middle of recording let's first store the clean in sound
                    if (this->isRecording) {
                        this->buffer.writeSample(in);
                    }
                    return in;
                }
                // This means the looper effect is "enabled"
                
                // Active: play or record
                T playback = (this->loopPlaybackActive) ? this->buffer.readSample(this->loopLength - this->loopPlaybackCursor) : 0;

                if (this->isRecording) {
                    if (this->loopPlaybackActive) {
                        // Overdub: input + current buffer contents
                        this->buffer.writeSample(in + playback);
                    }
                    else {
                        // First-time recording: write dry input
                        this->buffer.writeSample(in);
                        this->loopLength++; // measure length as we go
                    }
                }
                else if (this->loopPlaybackActive) {
                    // Playback-only: just advance buffer with dummy write
                    this->buffer.writeSample(this->buffer.readSample(this->loopLength - this->loopPlaybackCursor));
                }

                // Advance cursor
                if (this->loopPlaybackActive) {
                    this->loopPlaybackCursor++;
                    if (this->loopPlaybackCursor >= this->loopLength) {
                        this->loopPlaybackCursor = 0;
                    }
                }

                return giml::linMix(in, playback, this->blend);;
            }

            void setParams(float blend) {
                this->blend = blend;
            }

            void setIsRecording() {
                this->isRecording = true;
                if (!(this->loopPlaybackActive)) {
                    this->loopLength = 0;
                    this->loopPlaybackCursor = 0;
                }
            }

            void stopRecording() {
                this->isRecording = false;
                this->loopPlaybackCursor = 0; // Start playback at the beginning
                this->loopPlaybackActive = true; // Start playback
            }

            void reset() {
                this->loopLength = 0;
                this->loopPlaybackCursor = 0;
                this->loopPlaybackActive = false;
                this->isRecording = false;
            }
    }






} // namespace giml

#endif