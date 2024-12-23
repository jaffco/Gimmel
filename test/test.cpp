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

#include <chrono>
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
    WAVLoader loader { "audio/homemadeLick.wav" }; //Pick an input sound to test
    WAVWriter writer { "audio/out.wav", loader.sampleRate };

    giml::Reverb<float> r{ loader.sampleRate };
    r.setParams(0.020f, 0.6f, 0.75f, 10.f, 0.75f, giml::Reverb<float>::RoomType::CUBE);
    r.enable();


    float input, output;
    int i = 1;
    while (loader.readSample(&input)) { //Sample loop
        //Effects go here
        output = r.processSample(input);
        writer.writeSample(output); //Write modified sample to output file
        if (i % 100 == 0) {
            std::cout << "Wrote 100 samples" << std::endl;
        }
        i++;
    }

    return 0;
}