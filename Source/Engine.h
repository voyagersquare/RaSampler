#pragma once
#include "Parameters.h"
#include "SampleData.h"
#include <juce_dsp/juce_dsp.h>
namespace ra
{
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float tick(float x)
    {
        double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return float(y);
    }
    void reset()
    {
        z1 = z2 = 0;
    }
    void configure(int type, double rate, double freq, double q, double gain);
};
struct Voice
{
    bool active = false, held = false, pedal = false;
    int note = 60, channel = 1, direction = 1;
    bool releasing = false, reverseOrientation = false, releaseEntryPending = false;
    uint64_t age = 0;
    double pos = 0, pitch = 60, oscPhase = 0;
    float velocity = 1, random = 0, pressure = -1;
    double lfoPhase[2]{};
    float randomLfo[2]{};
    juce::ADSR amp, filter, pitchEnv;
    Biquad filters[2][2];
    std::shared_ptr<const SampleData> sample;
};
class Engine
{
  public:
    explicit Engine(APVTS &);
    void prepare(double, int);
    void reset();
    void process(juce::AudioBuffer<float> &, juce::MidiBuffer &, std::shared_ptr<const SampleData>,
                 double bpm);
    float peak = 0;
    std::atomic<int> voicesActive{0};

  private:
    APVTS &state;
    std::map<juce::String, std::atomic<float> *> parameters;
    std::map<juce::String, float> values;
    float p(const juce::String &id) const
    {
        return values.at(id);
    }
    double sr = 44100, bpm = 120;
    uint64_t counter = 0;
    std::array<Voice, 32> voices;
    std::array<float, 16> bends{}, aftertouch{}, wheel{};
    std::array<bool, 16> sustain{};
    float lastNote = 60;
    void event(const juce::MidiMessage &, std::shared_ptr<const SampleData>);
    void beginRelease(Voice &);
    void render(juce::AudioBuffer<float> &, int, int);
    void effects(juce::AudioBuffer<float> &);
    juce::AudioBuffer<float> delay, preDelay;
    int delayPos = 0, prePos = 0;
    float hpX[2]{}, hpY[2]{}, lpY[2]{};
    juce::Reverb reverb;
    std::array<juce::dsp::Chorus<float>, 3> chorus;
    juce::AudioBuffer<float> chorusDry, chorusWet;
    float phaserMemory[2][12]{}, phaserFeedback[2]{};
    double phaserPhase = 0;
};
} // namespace ra
