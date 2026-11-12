#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
namespace ra {
struct SampleData {
 juce::AudioBuffer<float> audio;
 double rate=44100;
 juce::String name="Init.wav";
 juce::MemoryBlock encoded;
 int length() const { return audio.getNumSamples(); }
};
std::shared_ptr<const SampleData> decodeSample(const void*,size_t,const juce::String&,juce::String&);
std::shared_ptr<const SampleData> stretchSample(const SampleData&,double factor);
std::shared_ptr<const SampleData> makeInitSample();
float snapToTransient(const SampleData&,float fraction);
}
