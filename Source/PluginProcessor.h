#pragma once
#include "Engine.h"
class RaSamplerProcessor final : public juce::AudioProcessor, private juce::Thread, private juce::AsyncUpdater
{
  public:
    RaSamplerProcessor();
    ~RaSamplerProcessor() override;
    ra::APVTS state;
    juce::MidiKeyboardState keyboard;
    const juce::String getName() const override
    {
        return "RaSampler";
    }
    void prepareToPlay(double, int) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
    bool isBusesLayoutSupported(const BusesLayout &) const override;
    bool acceptsMidi() const override
    {
        return true;
    }
    bool producesMidi() const override
    {
        return false;
    }
    bool isMidiEffect() const override
    {
        return false;
    }
    double getTailLengthSeconds() const override
    {
        return 30;
    }
    int getNumPrograms() override
    {
        return 1;
    }
    int getCurrentProgram() override
    {
        return 0;
    }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override
    {
        return "Default";
    }
    void changeProgramName(int, const juce::String &) override {}
    bool hasEditor() const override
    {
        return true;
    }
    juce::AudioProcessorEditor *createEditor() override;
    void getStateInformation(juce::MemoryBlock &) override;
    void setStateInformation(const void *, int) override;
    bool loadSample(const juce::File &, juce::String &);
    void setSample(std::shared_ptr<const ra::SampleData>);
    std::shared_ptr<const ra::SampleData> sample() const
    {
        return std::atomic_load(&playback);
    }
    std::shared_ptr<const ra::SampleData> originalSample() const
    {
        return std::atomic_load(&original);
    }
    void setParameter(const juce::String &, float);
    float value(const juce::String &) const;
    void requestStretch();
    void reorder(int, int);
    void resetParameters();
    void clearMappings();
    std::atomic<float> meter{0};
    std::atomic<int> activeVoices{0}, selectedTab{0}, learnTarget{-1}, lastLearned{-1};
    std::atomic<bool> panic{false}, stretchBusy{false}, waveformEnvelope{true};
    juce::String status() const;
    void setStatus(const juce::String &);

  private:
    ra::Engine engine;
    std::shared_ptr<const ra::SampleData> original, playback;
    std::atomic<int> revision{0};
    std::atomic<float> appliedStretch{1};
    std::array<std::atomic<int>, 128> mappings;
    std::unique_ptr<std::atomic<float>[]> pending;
    juce::CriticalSection messageLock;
    juce::String message = "Ready | Drop an audio file onto the waveform";
    void run() override;
    void handleAsyncUpdate() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RaSamplerProcessor)
};
