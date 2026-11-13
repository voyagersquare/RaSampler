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
// Measure actual rendered audio, including interpolation, instead of checking the pitch formula.
static double renderedFrequency(RaSamplerProcessor &processor, bool start = true, int midiNote = 60)
{
    juce::AudioBuffer<float> audio(2, 512);
    juce::MidiBuffer midi;
    if (start)
    {
        processor.panic = true;
        processor.processBlock(audio, midi);
        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1, midiNote, 1.f), 0);
    }
    float previous = 0;
    bool valid = false;
    double first = 0, last = 0;
    int crossings = 0;
    for (int block = 0; block < 40; ++block)
    {
        processor.processBlock(audio, midi);
        midi.clear();
        for (int i = 0; i < 512; ++i)
        {
            float x = audio.getSample(0, i);
            int at = block * 512 + i;
            if (at >= 1024 && valid && previous <= 0 && x > 0)
            {
                double crossing = at - 1 - double(previous) / (x - previous);
                if (crossings++ == 0)
                    first = crossing;
                last = crossing;
            }
            previous = x;
            valid = at >= 1024;
        }
    }
    if (crossings < 3 || last <= first)
        throw std::runtime_error("Not enough audio cycles to measure tuning");
    return (crossings - 1) * 48000. / (last - first);
}
static void checkRootNotes()
{
    RaSamplerProcessor processor;
    auto source = std::make_shared<ra::SampleData>();
    source->rate = 44100;
    source->name = "C3 calibration tone";
    source->audio.setSize(2, 44100);
    const double base = juce::MidiMessage::getMidiNoteInHertz(60);
    for (int i = 0; i < 44100; ++i)
        for (int ch = 0; ch < 2; ++ch)
            source->audio.setSample(
                ch, i, float(.3 * std::sin(juce::MathConstants<double>::twoPi * base * i / 44100.)));
    processor.setSample(source);
    processor.prepareToPlay(48000, 512);
    processor.setParameter("filterOn", 0);
    processor.setParameter("loop", 1);
    auto *root = processor.state.getParameter("root");
    check(root->getNumSteps() == 37, "Root Note exposes exactly 37 chromatic steps from C1 to C4");
    bool allPitch = true;
    double previous = 0;
    for (int note = 36; note <= 72; ++note)
    {
        processor.setParameter("root", float(note));
        double frequency = renderedFrequency(processor);
        double expected = base * std::pow(2., (note - 60) / 12.);
        if (std::abs(frequency / expected - 1) > .002 ||
            (previous > 0 && std::abs(frequency / previous - std::pow(2., 1. / 12.)) > .002))
            allPitch = false;
        previous = frequency;
    }
    check(allPitch, "all 37 Root Note steps produce ascending semitones in rendered sample audio");
    processor.setParameter("root", 60);
    double before = renderedFrequency(processor);
    processor.setParameter("root", 61);
    double after = renderedFrequency(processor, false);
    check(std::abs(after / before - std::pow(2., 1. / 12.)) < .002 && processor.activeVoices.load() == 1,
          "Root Note retunes an already playing sample without another MIDI note-on");
    processor.setParameter("root", 48);
    double midiSemitone = renderedFrequency(processor, true, 61);
    check(std::abs(midiSemitone / (base * std::pow(2., -11. / 12.)) - 1) < .002,
          "MIDI key tracking remains chromatic alongside Root Note tuning");
    processor.setParameter("transpose", 12);
    double transposed = renderedFrequency(processor);
    check(std::abs(transposed / base - 1) < .002, "Transpose combines correctly with Root Note");
    processor.setParameter("transpose", 0);
    auto silence = std::make_shared<ra::SampleData>(*source);
    silence->audio.clear();
    processor.setSample(silence);
    processor.setParameter("oscOn", 1);
    processor.setParameter("oscWave", 0);
    processor.setParameter("oscLevel", -6);
    bool oscillator = true;
    for (int note : {36, 60, 72})
    {
        processor.setParameter("root", float(note));
        if (std::abs(renderedFrequency(processor) / (base * std::pow(2., (note - 60) / 12.)) - 1) > .002)
            oscillator = false;
    }
    check(oscillator, "additional oscillator follows the selected Root Note");
    RaSamplerProcessor ui;
    RaSamplerEditor editor(ui);
    editor.selectTab(1);
    auto *canvas = editor.getChildComponent(0);
    RaControl *control = nullptr;
    RaKeyboard *keyboard = nullptr;
    for (int i = 0; i < canvas->getNumChildComponents(); ++i)
    {
        if (auto *c = dynamic_cast<RaControl *>(canvas->getChildComponent(i)))
            if (c->id == "root")
                control = c;
        if (auto *k = dynamic_cast<RaKeyboard *>(canvas->getChildComponent(i)))
            keyboard = k;
    }
    check(control && keyboard, "Root Note knob is connected to the native keyboard");
    const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    bool labels = true, highlights = true;
    for (int note = 36; note <= 72; ++note)
    {
        control->slider.setValue(note, juce::sendNotificationSync);
        juce::String label;
        for (int i = 0; i < control->slider.getNumChildComponents(); ++i)
            if (auto *value = dynamic_cast<juce::Label *>(control->slider.getChildComponent(i)))
                label = value->getText();
        auto expected = juce::String(names[(note - 36) % 12]) + juce::String(1 + (note - 36) / 12);
        if (label.replace(juce::String::charToString(0x266f), "#") != expected ||
            juce::roundToInt(ui.value("root")) != note)
            labels = false;
        auto bounds = keyboard->getRectangleForKey(note);
        auto image = keyboard->createComponentSnapshot(keyboard->getLocalBounds());
        int x = juce::jlimit(0, image.getWidth() - 1, int(bounds.getX() + bounds.getWidth() * .5f));
        int y = juce::jlimit(0, image.getHeight() - 1,
                             int(bounds.getBottom() - (juce::MidiMessage::isMidiNoteBlack(note) ? 14 : 4)));
        auto color = image.getPixelAt(x, y);
        if (color.getRed() < 180 || color.getGreen() < 100 || color.getBlue() > 150)
            highlights = false;
    }
    check(labels, "Root Note knob displays C, C sharp, D, D sharp, E, F, F sharp, G, G sharp, A, A sharp, B "
                  "across C1-C4");
    check(highlights, "each selected white or black keyboard key is filled yellow");
    ui.setParameter("root", 60);
    auto event =
        juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), {10, 10}, juce::ModifierKeys{}, 1,
                         0, 0, 0, 0, &control->slider, &control->slider, juce::Time::getCurrentTime(),
                         {10, 10}, juce::Time::getCurrentTime(), 0, false);
    juce::MouseWheelDetails wheel{};
    wheel.deltaY = .5f;
    static_cast<juce::Component &>(control->slider).mouseWheelMove(event, wheel);
    check(ui.value("root") == 61, "mouse wheel increases Root Note by exactly one semitone");
    wheel.deltaY = -.5f;
    static_cast<juce::Component &>(control->slider).mouseWheelMove(event, wheel);
    check(ui.value("root") == 60, "mouse wheel decreases Root Note by exactly one semitone");
    check(ra::parseRootNoteName("Db2") == 49 &&
              ra::parseRootNoteName(juce::String("D") + juce::String::charToString(0x266d) + "2") == 49 &&
              ra::parseRootNoteName("F#3") == 66,
          "note-name entry accepts sharp and flat enharmonic names");
    ui.setParameter("root", 0);
    check(ui.value("root") == 36, "Root Note clamps below C1");
    ui.setParameter("root", 127);
    check(ui.value("root") == 72, "Root Note clamps above C4");
    ui.setParameter("root", 71);
    keyboard->setLowestVisibleKey(0);
    control->slider.setValue(72, juce::sendNotificationSync);
    auto visible = keyboard->getRectangleForKey(72);
    check(visible.getX() >= 0 && visible.getRight() <= keyboard->getWidth(),
          "keyboard scrolls the selected root into view when necessary");
    juce::MemoryBlock preset;
    ui.getStateInformation(preset);
    auto tree = juce::ValueTree::readFromData(preset.getData(), preset.getSize());
    auto rootState = tree.getChildWithProperty("id", "root");
    check(rootState.isValid(), "root preset property remains compatible with existing parameter IDs");
    bool legacy = true;
    for (int old : {0, 127})
    {
        rootState.setProperty("value", old, nullptr);
        juce::MemoryBlock legacyData;
        {
            juce::MemoryOutputStream stream(legacyData, false);
            tree.writeToStream(stream);
        }
        RaSamplerProcessor restored;
        restored.setStateInformation(legacyData.getData(), int(legacyData.getSize()));
        if (restored.value("root") != float(old == 0 ? 36 : 72))
            legacy = false;
    }
    check(legacy, "older out-of-range root values are safely clamped when a preset is restored");
    ui.setParameter("root", 49);
    ui.getStateInformation(preset);
    RaSamplerProcessor restored;
    restored.setStateInformation(preset.getData(), int(preset.getSize()));
    check(restored.value("root") == 49, "selected chromatic root survives a preset round trip");
}
static std::shared_ptr<const ra::SampleData> loopFixture()
{
    auto sample = std::make_shared<ra::SampleData>();
    sample->name = "loop transport fixture";
    sample->rate = 48000;
    sample->audio.setSize(2, 128);
    for (int i = 0; i < 128; ++i)
        for (int ch = 0; ch < 2; ++ch)
            sample->audio.setSample(ch, i, float(i) / 128.f);
    return sample;
}
static void configureLoopTest(RaSamplerProcessor &p, int sustain, int release, bool linked = false,
                              bool reverse = false, bool master = true, int playback = 0,
                              float envelopeRelease = 1000)
{
    p.setParameter("transpose", 0);
    p.setParameter("root", 60);
    p.setParameter("releaseLoopStart", .5f);
    p.setSample(loopFixture());
    p.prepareToPlay(48000, 256);
    p.setParameter("loop", master ? 1 : 0);
    p.setParameter("loopStart", .25f);
    p.setParameter("loopEnd", .5f);
    p.setParameter("loopSustainMode", float(sustain));
    p.setParameter("loopReleaseMode", float(release));
    p.setParameter("loopLink", linked ? 1 : 0);
    p.setParameter("reverse", reverse ? 1 : 0);
    p.setParameter("playMode", float(playback));
    p.setParameter("filterOn", 0);
    p.setParameter("fadeIn", 0);
    p.setParameter("fadeOut", 0);
    p.setParameter("ampAttack", .1f);
    p.setParameter("ampRelease", envelopeRelease);
    juce::AudioBuffer<float> empty(2, 256);
    juce::MidiBuffer midi;
    p.processBlock(empty, midi);
}
static juce::AudioBuffer<float> loopAudio(RaSamplerProcessor &p, int noteOff = -1, bool pedal = false)
{
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 60, 1.f), 0);
    if (pedal)
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 60);
    if (noteOff >= 0)
        midi.addEvent(juce::MidiMessage::noteOff(1, 60), noteOff);
    if (pedal)
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 160);
    p.processBlock(audio, midi);
    return audio;
}
static int rampIndex(const juce::AudioBuffer<float> &audio, int at)
{
    return juce::roundToInt(audio.getSample(0, at) * 128);
}
static void checkLoopModes()
{
    RaSamplerProcessor p;
    configureLoopTest(p, 0, 1);
    auto once = loopAudio(p);
    check(rampIndex(once, 127) == 127 && once.getSample(0, 128) == 0 && p.activeVoices.load() == 0,
          "Sustain Once reaches the sample end without repeating");
    configureLoopTest(p, 1, 2);
    auto forward = loopAudio(p);
    check(rampIndex(forward, 62) == 62 && rampIndex(forward, 63) == 31 && rampIndex(forward, 64) == 32 &&
              rampIndex(forward, 95) == 31,
          "Sustain Forward wraps within LOOP IN and LOOP OUT");
    configureLoopTest(p, 2, 3);
    auto ping = loopAudio(p);
    check(rampIndex(ping, 63) == 63 && rampIndex(ping, 64) == 62 && rampIndex(ping, 95) == 31 &&
              rampIndex(ping, 96) == 32,
          "Sustain Ping-pong reflects at both loop boundaries without duplicate endpoints");
    configureLoopTest(p, 2, 3, false, true);
    auto reverse = loopAudio(p);
    check(rampIndex(reverse, 64) == 63 && rampIndex(reverse, 96) == 31 && rampIndex(reverse, 97) == 32,
          "Reverse starts ping-pong playback in the opposite direction");
    configureLoopTest(p, 1, 0);
    auto off = loopAudio(p, 80);
    check(rampIndex(off, 95) == 31 && rampIndex(off, 127) == 31 && p.activeVoices.load() == 1,
          "Release OFF continues the Sustain loop during amplitude ADSR release");
    configureLoopTest(p, 1, 1);
    auto tail = loopAudio(p, 80);
    check(rampIndex(tail, 80) == 48 && rampIndex(tail, 159) == 127 && tail.getSample(0, 160) == 0,
          "Release Once leaves the loop and plays toward the sample end");
    configureLoopTest(p, 1, 2);
    auto releaseForward = loopAudio(p, 80);
    check(rampIndex(releaseForward, 95) == 63 && rampIndex(releaseForward, 158) == 126 &&
              rampIndex(releaseForward, 159) == 63 && p.activeVoices.load() == 1,
          "Release Forward exits the Sustain loop and wraps from Sample End to Release Loop");
    configureLoopTest(p, 1, 3);
    auto releasePing = loopAudio(p, 80);
    check(rampIndex(releasePing, 159) == 127 && rampIndex(releasePing, 160) == 126 &&
              rampIndex(releasePing, 223) == 63 && rampIndex(releasePing, 224) == 64,
          "Release Ping-pong bounces between Sample End and its independent Release Loop start");
    configureLoopTest(p, 2, 0);
    auto linked = loopAudio(p, 80);
    check(rampIndex(linked, 81) == 45 && rampIndex(linked, 95) == 31 && rampIndex(linked, 96) == 32 &&
              p.activeVoices.load() == 1,
          "Release OFF preserves the current Sustain ping-pong direction");
    configureLoopTest(p, 1, 2, false, true);
    auto reverseRelease = loopAudio(p, 80);
    check(rampIndex(reverseRelease, 80) == 47 && rampIndex(reverseRelease, 127) == 0 &&
              rampIndex(reverseRelease, 128) == 62,
          "Reverse Release Forward exits toward Sample Start before wrapping into its Release region");
    configureLoopTest(p, 1, 3, false, true);
    auto reverseReleasePing = loopAudio(p, 80);
    check(rampIndex(reverseReleasePing, 127) == 0 && rampIndex(reverseReleasePing, 128) == 1 &&
              rampIndex(reverseReleasePing, 190) == 63 && rampIndex(reverseReleasePing, 191) == 62,
          "Reverse Release Ping-pong reflects between Sample Start and Release Loop");
    configureLoopTest(p, 1, 0, false, false, true, 0, 1);
    auto pedal = loopAudio(p, 80, true);
    check(pedal.getSample(0, 159) > 0 && pedal.getSample(0, 220) == 0 && p.activeVoices.load() == 0,
          "Release Mode waits until the sustain pedal is lifted");
    configureLoopTest(p, 1, 0, false, false, true, 1);
    auto trigger = loopAudio(p, 80);
    check(trigger.getSample(0, 200) > 0 && p.activeVoices.load() == 1,
          "One-shot playback continues to ignore ordinary Note Off");
    configureLoopTest(p, 1, 0, false, false, false);
    auto noLoop = loopAudio(p, 80);
    check(noLoop.getSample(0, 81) > 0 && noLoop.getSample(0, 128) == 0,
          "release loop modes do not alter playback when the main Loop switch is off");
    configureLoopTest(p, 1, 2, false, false, true, 0, 1);
    auto shortRelease = loopAudio(p, 80);
    check(shortRelease.getSample(0, 130) == 0 && p.activeVoices.load() == 0,
          "release looping ends when the amplitude ADSR Release finishes");
    configureLoopTest(p, 2, 3);
    p.setParameter("loopEnd", .26f);
    p.setParameter("transpose", 48);
    p.setParameter("root", 72);
    auto fast = loopAudio(p);
    bool bounded = true;
    for (int i = 32; i < 256; ++i)
        if (!std::isfinite(fast.getSample(0, i)) || rampIndex(fast, i) < 31 || rampIndex(fast, i) > 33)
            bounded = false;
    check(bounded,
          "ping-pong handles increments larger than a very short loop without reading outside its bounds");
    configureLoopTest(p, 1, 0, true);
    auto linkedStart = loopAudio(p);
    check(rampIndex(linkedStart, 16) == 47 && rampIndex(linkedStart, 31) == 62 &&
              rampIndex(linkedStart, 32) == 31,
          "Link starts playback at Sustain Loop Start instead of the stored Sample Start");
    check(p.value("start") == 0 && std::abs(p.effectiveSampleStart() - .25f) < .0001f,
          "Link preserves the original Sample Start parameter");
    configureLoopTest(p, 1, 0, true);
    p.setParameter("start", .4f);
    auto linkedEarlier = loopAudio(p);
    check(rampIndex(linkedEarlier, 16) == 47 && p.effectiveSampleStart() == .25f,
          "Link can move playback before the preserved original Sample Start");
    p.setParameter("start", 0);
    p.setParameter("loopLink", 0);
    check(p.effectiveSampleStart() == 0, "unlinking restores the original Sample Start");
    configureLoopTest(p, 2, 0, true, true);
    auto linkedReverse = loopAudio(p);
    check(rampIndex(linkedReverse, 16) == 47 && rampIndex(linkedReverse, 32) == 31 &&
              rampIndex(linkedReverse, 33) == 32,
          "Reverse Link starts at the opposite Sustain loop edge");
    RaSamplerProcessor ui;
    RaSamplerEditor editor(ui);
    editor.selectTab(1);
    RaLoopModes *modes = nullptr;
    RaControl *start = nullptr;
    RaWave *wave = nullptr;
    auto *canvas = editor.getChildComponent(0);
    for (int i = 0; i < canvas->getNumChildComponents(); ++i)
    {
        auto *child = canvas->getChildComponent(i);
        if (auto *item = dynamic_cast<RaLoopModes *>(child))
            modes = item;
        if (auto *item = dynamic_cast<RaControl *>(child))
            if (item->id == "start")
                start = item;
        if (auto *item = dynamic_cast<RaWave *>(child))
            wave = item;
    }
    check(modes && start && wave, "Sample page contains Sustain and Release controls inside LOOP");
    auto button = [&](const juce::String &name) -> juce::TextButton *
    {
        for (int i = 0; i < modes->getNumChildComponents(); ++i)
            if (auto *b = dynamic_cast<juce::TextButton *>(modes->getChildComponent(i)))
                if (b->getName() == name)
                    return b;
        throw std::runtime_error("Missing loop mode button");
    };
    check(!button("Sustain / Forward")->isEnabled(), "loop mode controls are inactive until Loop is enabled");
    ui.setParameter("loop", 1);
    ui.setParameter("loopStart", .25f);
    modes->refresh();
    button("Sustain / Ping-pong")->onClick();
    check(ui.value("loopSustainMode") == 2 && ui.value("loopReleaseMode") == 0,
          "Sustain and Release selections remain independent");
    button("Release / Forward")->onClick();
    button("Link Sample Start")->onClick();
    check(ui.value("loopLink") == 1 && ui.value("start") == 0 && !start->isEnabled() &&
              start->slider.getTextFromValue(0) ==
                  juce::String(juce::int64(.25 * (ui.sample()->length() - 1))),
          "Link disables the Sample Start field and displays its effective loop position");
    button("Release / Off")->onClick();
    check(ui.value("loopLink") == 1 && ui.value("loopSustainMode") == 2 && ui.value("loopReleaseMode") == 0,
          "Release OFF preserves Link and the Sustain selection");
    button("Link Sample Start")->onClick();
    check(start->isEnabled() && start->slider.getTextFromValue(0) == "0",
          "unlinking restores the editable original Sample Start field");
    button("Sustain / One-shot")->onClick();
    check(!button("Release / Forward")->isEnabled(), "Release requires an active Sustain loop");
    button("Sustain / Forward")->onClick();
    ui.setParameter("playMode", 1);
    modes->refresh();
    check(!button("Release / Forward")->isEnabled() && button("Sustain / Forward")->isEnabled(),
          "One-shot playback disables Release controls while Sustain remains available");
    ui.setParameter("playMode", 0);
    ui.setParameter("loopReleaseMode", 2);
    ui.setParameter("releaseLoopStart", .6f);
    ui.setParameter("snap", 0);
    modes->refresh();
    auto handle = wave->releaseHandleBounds().getCentre();
    auto event = [&](juce::Point<float> at)
    {
        return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), at,
                                juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, wave, wave,
                                juce::Time::getCurrentTime(), handle, juce::Time::getCurrentTime(), 1, true);
    };
    wave->mouseDown(event(handle));
    wave->mouseDrag(event(handle.translated(40, 0)));
    wave->mouseUp(event(handle.translated(40, 0)));
    check(std::abs(ui.value("releaseLoopStart") - (.6f + 40.f / wave->getWidth())) < .001f &&
              ui.value("loopStart") == .25f,
          "Release Loop waveform marker drags independently without moving the Sustain markers");
    ui.setParameter("loopSustainMode", 2);
    ui.setParameter("loopReleaseMode", 3);
    ui.setParameter("loopLink", 1);
    ui.setParameter("releaseLoopStart", .7f);
    juce::MemoryBlock state;
    ui.getStateInformation(state);
    RaSamplerProcessor restored;
    restored.setStateInformation(state.getData(), int(state.getSize()));
    check(restored.value("loopSustainMode") == 2 && restored.value("loopReleaseMode") == 3 &&
              restored.value("loopLink") == 1 && std::abs(restored.value("releaseLoopStart") - .7f) < .001f,
          "all loop modes, Link and Release Loop start survive a preset round trip");
    auto legacy = juce::ValueTree::readFromData(state.getData(), state.getSize());
    for (int i = legacy.getNumChildren() - 1; i >= 0; --i)
    {
        auto id = legacy.getChild(i).getProperty("id").toString();
        if (id == "loopSustainMode" || id == "loopReleaseMode" || id == "loopLink" ||
            id == "releaseLoopStart")
            legacy.removeChild(i, nullptr);
    }
    juce::MemoryBlock old;
    {
        juce::MemoryOutputStream stream(old, false);
        legacy.writeToStream(stream);
    }
    restored.setStateInformation(old.getData(), int(old.getSize()));
    check(restored.value("loopSustainMode") == 1 && restored.value("loopReleaseMode") == 0 &&
              restored.value("loopLink") == 0 && restored.value("releaseLoopStart") == .5f,
          "older presets retain their existing forward Sustain loop and release behavior");
    ui.globalReset();
    check(ui.value("loopSustainMode") == 1 && ui.value("loopReleaseMode") == 0 && ui.value("loopLink") == 0 &&
              ui.value("releaseLoopStart") == .5f,
          "Global Reset restores all default loop settings");
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
    check(editor.getWidth() == 1170 && editor.getHeight() == 555, "plugin opens at compact 75 percent size");
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
    uiProcessor.setParameter("ampRelease", 1);
    uiProcessor.setParameter("ampSustain", 0);
    auto shortReleaseHandles = waveform->envelopeHandles();
    check(shortReleaseHandles[2].getDistanceFrom(shortReleaseHandles[3]) >= 10,
          "Sustain and Release nodes remain independently selectable at minimum release");
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
    editor.selectTab(1);
    for (auto id : {"start", "end", "loop", "reverse", "snap"})
    {
        int upper = 0;
        for (int i = 0; i < canvas->getNumChildComponents(); ++i)
            if (auto *c = dynamic_cast<RaControl *>(canvas->getChildComponent(i)))
                if (c->id == id && c->getY() < 390)
                    ++upper;
        check(upper == 0 && findControl(id, true) != nullptr,
              "sample-only controls appear once on the Sample page");
    }
    uiProcessor.setParameter("loop", 1);
    uiProcessor.setParameter("loopStart", .2f);
    uiProcessor.setParameter("loopEnd", .8f);
    uiProcessor.setParameter("snap", 0);
    auto loopDrag = [&](int marker, juce::Point<float> delta)
    {
        auto start = waveform->loopHandleBounds()[size_t(marker)].getCentre();
        waveform->mouseDown(mouse(start, start));
        waveform->mouseDrag(mouse(start + delta, start, true));
        waveform->mouseUp(mouse(start + delta, start, true));
    };
    auto loopPoint = waveform->loopHandleBounds()[0].getCentre();
    waveform->mouseDown(mouse(loopPoint, loopPoint));
    waveform->mouseUp(mouse(loopPoint, loopPoint));
    check(std::abs(uiProcessor.value("loopStart") - .2f) < .00001,
          "clicking offset LOOP flag does not move the loop boundary");
    auto ampBefore = uiProcessor.value("ampAttack");
    loopDrag(0, {40, 0});
    check(std::abs(uiProcessor.value("loopStart") - (.2f + 40.f / waveform->getWidth())) < .001,
          "LOOP IN flag drags directly without Shift");
    loopDrag(1, {-40, 0});
    check(std::abs(uiProcessor.value("loopEnd") - (.8f - 40.f / waveform->getWidth())) < .001,
          "LOOP OUT flag drags directly without Shift");
    check(uiProcessor.value("start") == 0 && uiProcessor.value("end") == 1 &&
              uiProcessor.value("ampAttack") == ampBefore,
          "loop flags leave trim markers and ADSR unchanged");
    uiProcessor.setParameter("loopStart", .49f);
    uiProcessor.setParameter("loopEnd", .51f);
    auto tight = waveform->loopHandleBounds();
    check(!tight[0].intersects(tight[1]), "short loop uses separate rows for its two labels");
    loopDrag(0, {2000, 0});
    check(uiProcessor.value("loopStart") < uiProcessor.value("loopEnd"),
          "loop flag drag cannot cross the other loop boundary");
    uiProcessor.setParameter("loopStart", 0);
    uiProcessor.setParameter("loopEnd", 1);
    for (auto flag : waveform->loopHandleBounds())
        check(waveform->getLocalBounds().toFloat().contains(flag),
              "full-sample loop flags remain visible inside the waveform");
    auto shiftStart = juce::Point<float>(waveform->getWidth() * .1f, 40);
    waveform->mouseDown(mouse(shiftStart, shiftStart, false, juce::ModifierKeys::shiftModifier));
    waveform->mouseUp(mouse(shiftStart, shiftStart, false, juce::ModifierKeys::shiftModifier));
    check(std::abs(uiProcessor.value("loopStart") - .1f) < .001, "Shift-drag loop editing remains available");
    juce::ComboBox *sizes = nullptr;
    juce::TextButton *resetButton = nullptr;
    for (int i = 0; i < canvas->getNumChildComponents(); ++i)
    {
        auto *child = canvas->getChildComponent(i);
        if (auto *box = dynamic_cast<juce::ComboBox *>(child))
            if (box->getName() == "Interface Size")
                sizes = box;
        if (auto *button = dynamic_cast<juce::TextButton *>(child))
            if (button->getButtonText() == "Global Reset")
                resetButton = button;
    }
    check(sizes && resetButton, "footer exposes interface size and Global Reset");
    sizes->setSelectedId(100, juce::sendNotificationSync);
    check(editor.getWidth() == 1560 && editor.getHeight() == 740, "size menu enlarges plugin to 100 percent");
    sizes->setSelectedId(60, juce::sendNotificationSync);
    check(editor.getWidth() == 936 && editor.getHeight() == 444,
          "size menu supports a smaller 60 percent view");
    check(editor.getConstrainer() &&
              std::abs(editor.getConstrainer()->getFixedAspectRatio() - 1560. / 740.) < .001,
          "manual resizing preserves interface proportions");
    sizes->setSelectedId(100, juce::sendNotificationSync);
    auto retainedSample = p.originalSample();
    uiProcessor.setSample(retainedSample);
    uiProcessor.resetParameters();
    int cutoffTarget = 0;
    for (size_t i = 0; i < ra::specs().size(); ++i)
        if (ra::specs()[i].id == "cutoff")
            cutoffTarget = int(i);
    uiProcessor.learnTarget = cutoffTarget;
    juce::MidiBuffer resetMidi;
    juce::AudioBuffer<float> resetAudio(2, 512);
    resetMidi.addEvent(juce::MidiMessage::controllerEvent(1, 74, 100), 0);
    uiProcessor.processBlock(resetAudio, resetMidi);
    juce::MemoryBlock beforeReset;
    uiProcessor.getStateInformation(beforeReset);
    check(juce::ValueTree::readFromData(beforeReset.getData(), beforeReset.getSize())
              .getChildWithName("MidiMap")
              .hasProperty("cc74"),
          "Global Reset fixture includes a learned MIDI mapping");
    for (auto *parameter : uiProcessor.getParameters())
        parameter->setValueNotifyingHost(parameter->getDefaultValue() < .5f ? .8f : .2f);
    uiProcessor.waveformEnvelope = false;
    uiProcessor.keyboard.noteOn(1, 60, 1.f);
    resetButton->onClick();
    bool factory = true;
    for (auto *parameter : uiProcessor.getParameters())
        if (std::abs(parameter->getValue() - parameter->getDefaultValue()) > 1e-5f)
            factory = false;
    check(factory, "Global Reset restores every registered sound parameter and effect slot");
    check(uiProcessor.originalSample() == retainedSample &&
              uiProcessor.sample()->length() == retainedSample->length(),
          "Global Reset retains loaded audio and restores its original duration");
    check(editor.getWidth() == 1560 && editor.getHeight() == 740,
          "Global Reset retains chosen interface size");
    juce::MemoryBlock afterReset;
    uiProcessor.getStateInformation(afterReset);
    check(juce::ValueTree::readFromData(afterReset.getData(), afterReset.getSize())
                      .getChildWithName("MidiMap")
                      .getNumProperties() == 0 &&
              uiProcessor.learnTarget.load() == -1,
          "Global Reset clears MIDI mappings and cancels Learn");
    resetMidi.clear();
    uiProcessor.processBlock(resetAudio, resetMidi);
    check(energy(resetAudio) == 0 && uiProcessor.activeVoices.load() == 0 &&
              !uiProcessor.keyboard.isNoteOn(1, 60),
          "Global Reset stops voices and clears effect tails and keyboard notes");
    sizes->setSelectedId(60, juce::sendNotificationSync);
    editor.setSize(936, 444);
    auto small = editor.createComponentSnapshot(editor.getLocalBounds());
    check(small.getWidth() == 936 && small.getHeight() == 444, "native editor renders at minimum size");
    checkRootNotes();
    checkLoopModes();
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
        return "0.5.0";
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
        loopModesDemo = args.startsWith("--loop-modes-demo");
        rootDemo = args.startsWith("--root-demo");
        bool loopDemo = args.startsWith("--loop-demo");
        demo = args.startsWith("--envelope-demo") || loopDemo || rootDemo || loopModesDemo;
        directory = juce::File(args.fromFirstOccurrenceOf(loopModesDemo ? "--loop-modes-demo"
                                                          : rootDemo    ? "--root-demo"
                                                          : loopDemo    ? "--loop-demo"
                                                          : demo        ? "--envelope-demo"
                                                                        : "--screenshots",
                                                          false, false)
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
        if (loopDemo || loopModesDemo)
        {
            processor->setParameter("loop", 1);
            processor->setParameter("loopStart", .2f);
            processor->setParameter("loopEnd", .8f);
        }
        if (loopModesDemo)
        {
            processor->setParameter("loopSustainMode", 2);
            processor->setParameter("loopReleaseMode", 3);
            processor->setParameter("loopLink", 1);
            processor->setParameter("releaseLoopStart", .65f);
        }
        if (rootDemo)
            processor->setParameter("root", 61);
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
    bool demo = false, rootDemo = false, loopModesDemo = false;
    void timerCallback() override
    {
        int shownPage = (rootDemo || loopModesDemo) ? 1 : page;
        editor->selectTab(shownPage);
        auto image = editor->createComponentSnapshot(editor->getLocalBounds());
        auto file =
            directory.getChildFile(juce::String(shownPage) + "-" +
                                   juce::StringArray{"Effects", "Sample", "Pitch-Osc", "Filter-Global",
                                                     "Modulation", "MIDI"}[shownPage] +
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
