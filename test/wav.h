#pragma once
#ifndef WAV_H
#define WAV_H

#define DR_WAV_IMPLEMENTATION
#include "include/ezWav/dr_wav.h"

#include <iostream>
#include <stdlib.h> //For malloc

/* Opens WAV files in signed 32-bit format*/
class WAVLoader {
public:
    WAVLoader(const char* filename) {
        this->openWAVFile(filename);
    }
    ~WAVLoader() {
        if (pArr) {
            free(pArr);
        }
    }
    bool readSample(float* pFloat) {
        if (currentIndex < numberOfSamplesActuallyDecoded) {
            *pFloat = pArr[currentIndex];
            currentIndex++;
            return true;
        }
        this->restartPlayback(); //In case they want to loop it back
        *pFloat = 0;
        return false;
    }
    int sampleRate = 0;
private:
    float* pArr = nullptr;
    size_t currentIndex = 0, numberOfSamplesActuallyDecoded = 0;
    void restartPlayback() {
        this->currentIndex = 0;
    }
    void openWAVFile(const char* filename) {
        drwav wav;
        if (!drwav_init_file(&wav, filename)) {
            // Error opening WAV file.
            std::cout << "Could not open WAV file for reading: " << filename << std::endl;
            exit(0);
        }
        this->sampleRate = wav.sampleRate;
        int32_t* pDecodedInterleavedSamples = (int32_t*)malloc(wav.totalPCMFrameCount * wav.channels * sizeof(int32_t));
        numberOfSamplesActuallyDecoded = drwav_read_pcm_frames_s32(&wav, wav.totalPCMFrameCount, pDecodedInterleavedSamples);
        std::cout << "Channels: " << wav.channels << "\n\r Decoded: " << numberOfSamplesActuallyDecoded << "samples\r\nOther val: " << numberOfSamplesActuallyDecoded << std::endl;
        pArr = (float*)malloc(numberOfSamplesActuallyDecoded * sizeof(float));
        // Now we want to normalize the entire array
        normalizeArr(pDecodedInterleavedSamples, numberOfSamplesActuallyDecoded, pArr);
        free(pDecodedInterleavedSamples);
        drwav_uninit(&wav);
    }
    void normalizeArr(int32_t* pArrIn, int size, float* pArrOut) {
        for (int i = 0; i < size; i++) {
            pArrOut[i] = pArrIn[i] / static_cast<double>(2147483647.f); //Divide by 32-bit
        }
    }
};

class WAVWriter {
private:
    drwav* pWAV = nullptr;
public:
    WAVWriter(const char* filename, int sampleRate = 48000) {

        //if (!drwav_init_file(pWAV, filename, NULL)) {
        //    // Error opening WAV file.
        //    std::cout << "Could not open WAV file for reading: " << filename << std::endl;
        //    exit(0);
        //}

        pWAV = (drwav*)malloc(sizeof(drwav));
        drwav_data_format wavFormat;

        wavFormat.bitsPerSample = 32;
        wavFormat.channels = 1;
        wavFormat.container = drwav_container_riff;
        wavFormat.format = DR_WAVE_FORMAT_PCM;
        wavFormat.sampleRate = sampleRate;

        drwav_init_file_write(pWAV, filename, &wavFormat);
        if (!pWAV) {
            //Could not open the file
            std::cout << "Could not open WAV file for writing: " << filename << std::endl;
            exit(0);
        }
    }
    ~WAVWriter() {
        if (pWAV) {
            drwav_uninit(pWAV);
        }
    }
    void writeSample(float f) {
        int32_t* pConvertedOut = new int32_t;
        *pConvertedOut = static_cast<int32_t>(f * 2147483647.f); //Convert the float to a int32 for WAV format
        drwav_write_pcm_frames(pWAV, 1, pConvertedOut);
        delete pConvertedOut;
    }

};
#endif