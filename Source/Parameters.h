#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
namespace ra {
using APVTS = juce::AudioProcessorValueTreeState;
APVTS::ParameterLayout makeParameters();
struct ParamSpec { juce::String id, label, unit; float min, max, initial, skew; juce::StringArray choices; bool toggle = false; };
const std::vector<ParamSpec>& specs();
}
