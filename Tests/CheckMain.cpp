#include "../Source/PluginEditor.h"
#include <iostream>
#include <stdexcept>
static void check(bool ok, const char *message)
{
    if (!ok)
        throw std::runtime_error(message);
    std::cout << "PASS " << message << std::endl;
}
static double energy(const juce::AudioBuffer<float> &b)
{
    double sum = 0;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            float v = b.getSample(ch, i);
            if (!std::isfinite(v))
                throw std::runtime_error("non-finite output");
            sum += v * v;
        }
    return sum;
}
static void test()
{
    RaSamplerProcessor p;
    p.prepareToPlay(48000, 512);
    juce::AudioBuffer<float> b(2, 512);
    juce::MidiBuffer midi;
    p.processBlock(b, midi);
    check(energy(b) == 0, "silence before note-on");
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, .9f), 64);
    p.processBlock(b, midi);
    check(energy(b) > 0, "sample note produces audio");
    bool silent = true;
    for (int i = 0; i < 64; ++i)
        if (b.getSample(0, i) != 0)
            silent = false;
    check(silent, "sample-accurate MIDI offset");
    midi.clear();
    p.panic = true;
    p.processBlock(b, midi);
    check(energy(b) == 0, "panic silences voices");
    p.setParameter("loop", 1);
    p.setParameter("loopStart", .1f);
    p.setParameter("loopEnd", .4f);
    p.setParameter("reverse", 1);
    p.setParameter("oscOn", 1);
    p.setParameter("filterType", 6);
    p.setParameter("peqGain", 9);
    p.setParameter("mod0Source", 1);
    p.setParameter("mod0Dest", 1);
    p.setParameter("mod0Amount", .4f);
    midi.addEvent(juce::MidiMessage::noteOn(1, 64, .8f), 0);
    p.processBlock(b, midi);
    midi.clear();
    double total = 0;
    for (int i = 0; i < 160; ++i)
    {
        p.processBlock(b, midi);
        total += energy(b);
    }
    check(total > 0 && p.activeVoices.load() > 0, "reverse loop and oscillator sustain beyond sample length");
    for (auto fx : {"delayOn", "reverbOn", "chorusOn", "phaserOn", "saturationOn"})
        p.setParameter(fx, 1);
    for (int type = 0; type < 7; ++type)
    {
        p.setParameter("filterType", float(type));
        for (int j = 0; j < 4; ++j)
        {
            p.processBlock(b, midi);
            check(energy(b) < 1e8, "filter and effect chain output remains finite");
        }
    }
    p.reorder(0, 4);
    check(int(p.value("fxOrder0")) == 1 && int(p.value("fxOrder4")) == 0, "effect reorder preserves chain");
    juce::MemoryBlock saved;
    p.getStateInformation(saved);
    RaSamplerProcessor restored;
    restored.setStateInformation(saved.getData(), int(saved.getSize()));
    check(restored.value("reverse") > .5f && restored.value("loop") > .5f &&
              std::abs(restored.value("peqGain") - 9.f) < .0001f,
          "DAW state restores parameters");
    auto original = ra::makeInitSample();
    auto stretched = ra::stretchSample(*original, 1.5);
    check(stretched->length() == int(original->length() * 1.5),
          "time-stretch changes length by requested factor");
    check(energy(stretched->audio) > 0, "time-stretch produces finite nonzero audio");
    auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getNonexistentChildFile("rasampler-test", ".wav");
    juce::WavAudioFormat wav;
    {
        auto out = file.createOutputStream();
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wav.createWriterFor(out.release(), 44100, 2, 16, {}, 0));
        check(bool(writer), "WAV fixture writer");
        writer->writeFromAudioSampleBuffer(original->audio, 0, original->length());
    }
    juce::String error;
    check(p.loadSample(file, error), "load external WAV");
    p.getStateInformation(saved);
    file.deleteFile();
    restored.setStateInformation(saved.getData(), int(saved.getSize()));
    check(restored.originalSample()->encoded.getSize() > 0 &&
              restored.originalSample()->length() == original->length(),
          "sample embedded and restored after source file deleted");
    p.setParameter("playMode", 0);
    p.setParameter("loop", 1);
    p.setParameter("ampRelease", 1);
    p.panic = true;
    midi.clear();
    p.processBlock(b, midi);
    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.f), 0);
    p.processBlock(b, midi);
    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
    for (int i = 0; i < 8; ++i)
    {
        p.processBlock(b, midi);
        midi.clear();
    }
    check(p.activeVoices.load() == 0, "gate note-off releases voice");
    p.panic = true;
    midi.clear();
    p.processBlock(b, midi);
    midi.clear();
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 40);
    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 100);
    p.processBlock(b, midi);
    check(p.activeVoices.load() == 1, "sustain pedal holds released note");
    midi.clear();
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 0);
    p.processBlock(b, midi);
    check(p.activeVoices.load() == 0, "pedal release releases held note");
    p.setParameter("polyphony", 2);
    midi.clear();
    for (int note = 60; note < 63; ++note)
        midi.addEvent(juce::MidiMessage::noteOn(1, note, .7f), 0);
    p.processBlock(b, midi);
    check(p.activeVoices.load() == 2, "polyphony limit and voice stealing");
    p.panic = true;
    midi.clear();
    p.processBlock(b, midi);
    midi.clear();
    p.setParameter("playMode", 1);
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, .8f), 0);
    midi.addEvent(juce::MidiMessage::noteOff(1, 60), 100);
    p.processBlock(b, midi);
    check(p.activeVoices.load() == 1, "one-shot ignores ordinary note-off");
    int target = 0;
    for (size_t i = 0; i < ra::specs().size(); ++i)
        if (ra::specs()[i].id == "cutoff")
            target = int(i);
    p.learnTarget = target;
    midi.clear();
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 74, 100), 0);
    p.processBlock(b, midi);
    p.getStateInformation(saved);
    auto learned = juce::ValueTree::readFromData(saved.getData(), saved.getSize());
    check(learned.getChildWithName("MidiMap").getProperty("cc74").toString() == "cutoff",
          "CC learn mapping stored by parameter ID");
    restored.setStateInformation(saved.getData(), int(saved.getSize()));
    restored.getStateInformation(saved);
    restored.setStateInformation(saved.getData(), int(saved.getSize()));
    restored.getStateInformation(saved);
    auto repeated = juce::ValueTree::readFromData(saved.getData(), saved.getSize());
    int maps = 0;
    for (auto child : repeated)
        if (child.hasType("MidiMap"))
            ++maps;
    check(maps == 1, "repeated state round trips do not duplicate MIDI mappings");
    ra::SampleData sine;
    sine.rate = 44100;
    sine.audio.setSize(2, 44100);
    for (int i = 0; i < 44100; ++i)
        for (int c = 0; c < 2; ++c)
            sine.audio.setSample(c, i, .3f * std::sin(juce::MathConstants<double>::twoPi * 440 * i / 44100));
    auto longSine = ra::stretchSample(sine, 1.5);
    int crossings = 0;
    for (int i = 4097; i < longSine->length() - 4096; ++i)
        if (longSine->audio.getSample(0, i - 1) <= 0 && longSine->audio.getSample(0, i) > 0)
            ++crossings;
    double measured = crossings * 44100. / (longSine->length() - 8193);
    check(std::abs(measured - 440) < 8, "time-stretch retains sine-wave pitch");
    std::cout << "All RaSampler checks passed." << std::endl;
}
class CheckApplication : public juce::JUCEApplication, private juce::Timer
{
  public:
    const juce::String getApplicationName() override
    {
        return "RaSamplerCheck";
    }
    const juce::String getApplicationVersion() override
    {
        return "0.1";
    }
    void initialise(const juce::String &args) override
    {
        if (args.startsWith("--self-test"))
        {
            try
            {
                test();
                setApplicationReturnValue(0);
            }
            catch (const std::exception &e)
            {
                std::cerr << "FAIL " << e.what() << std::endl;
                setApplicationReturnValue(1);
            }
            quit();
            return;
        }
        directory = juce::File(args.fromFirstOccurrenceOf("--screenshots", false, false).trim().unquoted());
        directory.createDirectory();
        processor = std::make_unique<RaSamplerProcessor>();
        processor->prepareToPlay(48000, 512);
        editor = std::make_unique<RaSamplerEditor>(*processor);
        editor->addToDesktop(0);
        editor->setVisible(true);
        startTimer(350);
    }
    void shutdown() override
    {
        stopTimer();
        editor.reset();
        processor.reset();
    }
    void systemRequestedQuit() override
    {
        quit();
    }

  private:
    std::unique_ptr<RaSamplerProcessor> processor;
    std::unique_ptr<RaSamplerEditor> editor;
    juce::File directory;
    int page = 0;
    void timerCallback() override
    {
        editor->selectTab(page);
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        auto file = directory.getChildFile(
            juce::String(page) + "-" +
            juce::StringArray{"Effects", "Sample", "Pitch-Osc", "Filter-Global", "Modulation", "MIDI"}[page] +
            ".png");
        auto out = file.createOutputStream();
        juce::PNGImageFormat png;
        if (!out || !png.writeImageToStream(image, *out))
        {
            setApplicationReturnValue(1);
            quit();
            return;
        }
        std::cout << file.getFullPathName() << std::endl;
        if (++page == 6)
        {
            stopTimer();
            quit();
        }
    }
};
START_JUCE_APPLICATION(CheckApplication)
