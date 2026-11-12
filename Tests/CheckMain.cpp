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
    RaSamplerProcessor uiProcessor;
    RaSamplerEditor editor(uiProcessor);
    auto findControl = [&](const juce::String &id, bool lower) -> RaControl *
    {
        auto *canvas = editor.getChildComponent(0);
        for (int i = 0; i < canvas->getNumChildComponents(); ++i)
            if (auto *c = dynamic_cast<RaControl *>(canvas->getChildComponent(i)))
                if (c->id == id && (!lower || c->getY() > 390))
                    return c;
        return nullptr;
    };
    for (int tab = 0; tab < 6; ++tab)
    {
        editor.selectTab(tab);
        check(uiProcessor.selectedTab.load() == tab, "native editor tab selection");
    }
    editor.selectTab(1);
    auto *endControl = findControl("end", true);
    juce::String endText;
    if (endControl)
        for (int i = 0; i < endControl->slider.getNumChildComponents(); ++i)
            if (auto *label = dynamic_cast<juce::Label *>(endControl->slider.getChildComponent(i)))
                endText = label->getText();
    check(endText == juce::String(uiProcessor.sample()->length() - 1),
          "sample editor displays positions in samples on first opening");
    auto *reverseControl = findControl("reverse", true);
    check(reverseControl != nullptr, "sample editor exposes reverse control");
    reverseControl->toggle.setToggleState(true, juce::sendNotificationSync);
    check(uiProcessor.value("reverse") > .5f, "native toggle updates audio parameter");
    editor.selectTab(0);
    auto *delayControl = findControl("delayTime", true);
    delayControl->slider.setValue(450, juce::sendNotificationSync);
    check(std::abs(uiProcessor.value("delayTime") - 450) < .01, "native knob updates audio parameter");
    RaWave *waveform = nullptr;
    auto *canvas = editor.getChildComponent(0);
    for (int i = 0; i < canvas->getNumChildComponents(); ++i)
        if (auto *candidate = dynamic_cast<RaWave *>(canvas->getChildComponent(i)))
            waveform = candidate;
    check(waveform != nullptr, "main sample view exposes ADSR overlay");
    auto mouse =
        [&](juce::Point<float> position, juce::Point<float> down, bool dragged = false, int extra = 0)
    {
        return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), position,
                                juce::ModifierKeys(juce::ModifierKeys::leftButtonModifier | extra), 1, 0, 0,
                                0, 0, waveform, waveform, juce::Time::getCurrentTime(), down,
                                juce::Time::getCurrentTime(), 1, dragged);
    };
    auto gesture = [&](int handle, juce::Point<float> delta)
    {
        auto start = waveform->envelopeHandles()[size_t(handle)];
        waveform->mouseDown(mouse(start, start));
        waveform->mouseDrag(mouse(start + delta, start, true));
        waveform->mouseUp(mouse(start + delta, start, true));
    };
    float oldAttack = uiProcessor.value("ampAttack");
    auto point = waveform->envelopeHandles()[0];
    waveform->mouseDown(mouse(point, point));
    waveform->mouseUp(mouse(point, point));
    check(uiProcessor.value("ampAttack") == oldAttack, "clicking ADSR node does not change the sound");
    gesture(0, {80, 0});
    check(uiProcessor.value("ampAttack") > oldAttack + 20, "waveform Attack drag updates amplitude envelope");
    gesture(1, {25, 18});
    check(uiProcessor.value("ampDecay") > 200 && uiProcessor.value("ampSustain") < .9,
          "Decay node controls decay time and sustain level");
    float oldSustain = uiProcessor.value("ampSustain");
    gesture(2, {0, 10});
    check(uiProcessor.value("ampSustain") < oldSustain, "waveform Sustain drag updates level");
    float oldRelease = uiProcessor.value("ampRelease");
    gesture(3, {-30, 0});
    check(uiProcessor.value("ampRelease") > oldRelease + 20, "waveform Release drag updates release time");
    check(uiProcessor.value("start") == 0 && uiProcessor.value("end") == 1,
          "ADSR editing preserves sample boundaries");
    editor.selectTab(3);
    auto *attackKnob = findControl("ampAttack", true);
    check(attackKnob && std::abs(attackKnob->slider.getValue() - uiProcessor.value("ampAttack")) < .001,
          "waveform and amplitude knobs stay synchronized");
    auto before = waveform->envelopeHandles()[3];
    findControl("ampRelease", true)->slider.setValue(300, juce::sendNotificationSync);
    check(waveform->envelopeHandles()[3].getDistanceFrom(before) > 1,
          "amplitude knob updates waveform envelope geometry");
    uiProcessor.waveformEnvelope = false;
    juce::MemoryBlock envelopeState;
    uiProcessor.getStateInformation(envelopeState);
    RaSamplerProcessor envelopeRestored;
    envelopeRestored.setStateInformation(envelopeState.getData(), int(envelopeState.getSize()));
    check(!envelopeRestored.waveformEnvelope.load() &&
              std::abs(envelopeRestored.value("ampAttack") - uiProcessor.value("ampAttack")) < .001,
          "preset restores ADSR parameters and overlay visibility");
    auto legacy = juce::ValueTree::readFromData(envelopeState.getData(), envelopeState.getSize());
    legacy.removeProperty("waveformEnvelope", nullptr);
    juce::MemoryBlock legacyState;
    {
        juce::MemoryOutputStream stream(legacyState, false);
        legacy.writeToStream(stream);
    }
    envelopeRestored.setStateInformation(legacyState.getData(), int(legacyState.getSize()));
    check(envelopeRestored.waveformEnvelope.load(), "older presets open with the ADSR overlay available");
    uiProcessor.waveformEnvelope = true;
    gesture(0, {-20000, 0});
    gesture(3, {-20000, 0});
    check(uiProcessor.value("ampAttack") >= .1f && uiProcessor.value("ampRelease") <= 10000,
          "ADSR drags clamp to legal parameter ranges");
    uiProcessor.setParameter("start", .999f);
    for (auto h : waveform->envelopeHandles())
        check(std::isfinite(h.x) && std::isfinite(h.y) && waveform->getLocalBounds().toFloat().contains(h),
              "ADSR handles remain accessible for very short sample selections");
    uiProcessor.setParameter("start", 0);
    uiProcessor.setParameter("snap", 0);
    auto trim = juce::Point<float>(waveform->getWidth() * .2f, float(waveform->getHeight() - 1));
    waveform->mouseDown(mouse(trim, trim));
    waveform->mouseUp(mouse(trim, trim));
    check(std::abs(uiProcessor.value("start") - .2f) < .001,
          "sample trimming still works outside ADSR nodes");
    uiProcessor.setParameter("start", 0);
    uiProcessor.setParameter("ampAttack", 200);
    uiProcessor.setParameter("reverse", 0);
    uiProcessor.prepareToPlay(48000, 512);
    RaSamplerProcessor fastEnvelope;
    fastEnvelope.prepareToPlay(48000, 512);
    juce::AudioBuffer<float> fastAudio(2, 512), slowAudio(2, 512);
    juce::MidiBuffer fastMidi, slowMidi;
    fastMidi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.f), 0);
    slowMidi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.f), 0);
    fastEnvelope.processBlock(fastAudio, fastMidi);
    uiProcessor.processBlock(slowAudio, slowMidi);
    check(energy(slowAudio) > 0 && energy(slowAudio) < energy(fastAudio) * .2,
          "amplitude Attack setting audibly changes rendered samples");
    editor.setSize(1170, 555);
    auto small = editor.createComponentSnapshot(editor.getLocalBounds());
    check(small.getWidth() == 1170 && small.getHeight() == 555, "native editor renders at minimum size");
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
        return "0.2";
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
        demo = args.startsWith("--envelope-demo");
        directory =
            juce::File(args.fromFirstOccurrenceOf(demo ? "--envelope-demo" : "--screenshots", false, false)
                           .trim()
                           .unquoted());
        directory.createDirectory();
        processor = std::make_unique<RaSamplerProcessor>();
        processor->prepareToPlay(48000, 512);
        if (demo)
        {
            processor->setParameter("ampAttack", 60);
            processor->setParameter("ampDecay", 90);
            processor->setParameter("ampSustain", .8f);
            processor->setParameter("ampRelease", 80);
        }
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
    bool demo = false;
    void timerCallback() override
    {
        editor->selectTab(page);
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        auto file = directory.getChildFile(
            juce::String(page) + "-" +
            juce::StringArray{"Effects", "Sample", "Pitch-Osc", "Filter-Global", "Modulation", "MIDI"}[page] +
            ".png");
        auto out = file.createOutputStream();
        if (out)
        {
            out->setPosition(0);
            out->truncate();
        }
        juce::PNGImageFormat png;
        if (!out || !png.writeImageToStream(image, *out))
        {
            setApplicationReturnValue(1);
            quit();
            return;
        }
        std::cout << file.getFullPathName() << std::endl;
        if (++page == (demo ? 1 : 6))
        {
            stopTimer();
            quit();
        }
    }
};
START_JUCE_APPLICATION(CheckApplication)
