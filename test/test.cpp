#include "wav.h"

#include "JaffMalloc.hpp"

//Overwrite stdlib calls in giml space
Jaffx::MyMalloc m;
namespace giml {
    void* malloc(size_t size) {
        return m.malloc(size);
    }

    void* calloc(size_t nelemb, size_t size) {
        return m.calloc(nelemb, size);
    }

    void* realloc(void* ptr, size_t size) {
        return m.realloc(ptr, size);
    }

    void free(void* ptr) {
        m.free(ptr);
    }
}
#include "../include/reverb.hpp"
#include "../include/oscillator.hpp"


#include <chrono>
#include <vector>


static long long timeElapsed = 0L;
static long long iterations = 0L;

#define BENCHMARK_CODE_AVG(code) { \
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now(); \
        code \
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now(); \
        timeElapsed += std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count(); \
        iterations++; \
        if (iterations > 48000 * 5) { \
			std::cout << "average " << timeElapsed / iterations << "ns avg every 5 sec" << std::endl; \
			timeElapsed = iterations = 0; \
		} \
   }

int main() {

    /*giml::DynamicArray<float> a;
    a.pushBack(67);
    std::cout << a.size() << std::endl;
    std::cout << a.getCapacity() << std::endl;
    for (int i = 0; i < 10; i++) {
        a.pushBack(i * 0.9f);
    }
    
    giml::DynamicArray<float> b = a;

    b.removeAt(4);
    a.removeAt(3);
    b.removeAt(7);


    for (int i = 0; i < a.size(); i++) {
        assert(a[i] == b[i]);
    }

    for (int i = 0; i < 12; i++) {
        std::cout << a.popBack() << std::endl;
    }

    std::cout << "A:" << std::endl;
    for (const float& f : a) {
        std::cout << a.popBack() << std::endl;
    }

    std::cout << "B:" << std::endl;
    for (const float& f : b) {
        std::cout << b.popBack() << std::endl;
    }*/
    m.MyMallocInit();
    WAVLoader loader { "audio/vocals.wav" }; //Pick an input sound to test
    WAVWriter writer { "audio/out.wav", loader.sampleRate };
    std::unique_ptr<giml::Reverb<float>> r;
    r = std::make_unique<giml::Reverb<float>>(loader.sampleRate, 4, 20, 4, 2);
    //giml::Reverb<float> r{ loader.sampleRate };
    r->setParams(0.030f, 0.6f, 0.75f, 1000.f, 0.5f, giml::Reverb<float>::RoomType::CUBE);
    r->enable();

 //   giml::TriOsc<float> sinOsc{ 48000 };
	//sinOsc.setFrequency(220.f);

    float input, output;
    int i = 1;
    std::vector<float> a;
    while (loader.readSample(&input)) { //Sample loop
        //Effects go here
        output = giml::powMix<float>(r->processSample(input), input, 0.25);
		//output = sinOsc.processSample();
        writer.writeSample(output); //Write modified sample to output file
        a.push_back(output);
        if (i % 10000 == 0) {
            std::cout << "Wrote 10000 samples" << std::endl;
        }
        i++;
    }

    return 0;
}