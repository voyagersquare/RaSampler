#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
namespace ra
{
using APVTS = juce::AudioProcessorValueTreeState;
inline constexpr int rootMinNote = 36, rootMaxNote = 72, rootReferenceNote = 60;
juce::String rootNoteName(int midiNote);
int parseRootNoteName(const juce::String &text);
APVTS::ParameterLayout makeParameters();
int effectiveReleaseMode(int sustainMode, int releaseMode, bool linked);
struct ParamSpec
{
    juce::String id, label, unit;
    float min, max, initial, skew;
    juce::StringArray choices;
    bool toggle = false;
};
const std::vector<ParamSpec> &specs();
} // namespace ra
