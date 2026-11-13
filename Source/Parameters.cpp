#include "Parameters.h"
namespace ra
{
juce::String rootNoteName(int note)
{
    return juce::MidiMessage::getMidiNoteName(juce::jlimit(rootMinNote, rootMaxNote, note), true, true, 3)
        .replace("#", juce::String::charToString(0x266f));
}
int parseRootNoteName(const juce::String &text)
{
    auto name = text.trim().replaceCharacter(0x266f, '#').replaceCharacter(0x266d, 'b').removeCharacters(" ");
    for (int note = 0; note < 128; ++note)
        if (name.equalsIgnoreCase(juce::MidiMessage::getMidiNoteName(note, true, true, 3)) ||
            name.equalsIgnoreCase(juce::MidiMessage::getMidiNoteName(note, false, true, 3)))
            return juce::jlimit(rootMinNote, rootMaxNote, note);
    if (name.isNotEmpty() && name.containsOnly("+-0123456789."))
        return juce::roundToInt(
            juce::jlimit(double(rootMinNote), double(rootMaxNote), name.getDoubleValue()));
    return rootReferenceNote;
}
const std::vector<ParamSpec> &specs()
{
    static const auto list = []
    {
        std::vector<ParamSpec> p;
        auto f = [&](const char *id, const char *name, float lo, float hi, float value, const char *unit = "",
                     float skew = 1.f) { p.push_back({id, name, unit, lo, hi, value, skew, {}, false}); };
        auto b = [&](const char *id, const char *name, bool value)
        { p.push_back({id, name, "", 0, 1, value ? 1.f : 0.f, 1, {}, true}); };
        auto c = [&](const char *id, const char *name, juce::StringArray options, int value = 0)
        { p.push_back({id, name, "", 0, float(options.size() - 1), float(value), 1, options, false}); };
        f("start", "Start", 0, 1, 0);
        f("end", "End", 0, 1, 1);
        f("loopStart", "Loop start", 0, 1, 0);
        f("loopEnd", "Loop end", 0, 1, 1);
        b("loop", "Loop", false);
        b("reverse", "Reverse", false);
        b("snap", "Snap", true);
        f("fadeIn", "Fade In", 0, 500, 0, "ms", .4f);
        f("fadeOut", "Fade Out", 0, 500, 3.5f, "ms", .4f);
        f("sampleGain", "Gain", -48, 24, 0, "dB");
        f("root", "Root Note", rootMinNote, rootMaxNote, rootReferenceNote);
        c("playMode", "Playback", {"Gate", "One-shot"});
        f("stretch", "Time stretch", .25f, 4, 1, "x", .5f);
        f("transpose", "Transpose", -48, 48, 0, "st");
        f("detune", "Detune", -100, 100, 0, "ct");
        f("pitchAttack", "Attack", .1f, 10000, 1, "ms", .3f);
        f("pitchDecay", "Decay", 1, 10000, 120, "ms", .3f);
        f("pitchSustain", "Sustain", 0, 1, 0);
        f("pitchRelease", "Release", 1, 10000, 100, "ms", .3f);
        f("pitchAmount", "Amount", -48, 48, 0, "st");
        b("oscOn", "Oscillator", false);
        c("oscWave", "Wave", {"Sine", "Square", "Saw", "Noise"});
        f("oscLevel", "Level", -60, 0, -18, "dB");
        f("oscTune", "Tune", -36, 36, 0, "st");
        b("filterOn", "Filter", true);
        c("filterType", "Type", {"LP 12 dB", "LP 24 dB", "HP 12 dB", "HP 24 dB", "BP", "Notch", "PEQ"}, 1);
        f("cutoff", "Freq", 20, 20000, 12000, "Hz", .25f);
        f("resonance", "Res", .1f, 10, .707f, "Q", .45f);
        f("drive", "Drive", 0, 24, 0, "dB");
        f("peqGain", "EQ Gain", -24, 24, 0, "dB");
        f("volume", "Volume", -60, 12, 0, "dB");
        f("pan", "Pan", -1, 1, 0);
        f("polyphony", "Polyphony", 1, 32, 16);
        f("glide", "Glide", 0, 2000, 0, "ms", .4f);
        f("ampAttack", "Attack", .1f, 10000, 1, "ms", .3f);
        f("ampDecay", "Decay", 1, 10000, 200, "ms", .3f);
        f("ampSustain", "Sustain", 0, 1, 1);
        f("ampRelease", "Release", 1, 10000, 100, "ms", .3f);
        f("filterAttack", "Attack", .1f, 10000, 1, "ms", .3f);
        f("filterDecay", "Decay", 1, 10000, 250, "ms", .3f);
        f("filterSustain", "Sustain", 0, 1, 0);
        f("filterRelease", "Release", 1, 10000, 100, "ms", .3f);
        f("filterAmount", "Amount", -8, 8, 0, "oct");
        f("velocityCurve", "Velocity", .2f, 4, 1);
        f("bendRange", "Bend range", 0, 24, 2, "st");
        c("midiChannel", "Channel",
          {"Omni", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16"});
        b("sustainEnable", "Sustain pedal", true);
        b("aftertouchEnable", "Aftertouch", true);
        for (int n = 1; n <= 2; ++n)
        {
            auto prefix = "lfo" + juce::String(n);
            auto id = prefix + "Rate";
            f(id.toRawUTF8(), "Rate", .01f, 30, n == 1 ? 1.f : .3f, "Hz", .4f);
            id = prefix + "Wave";
            c(id.toRawUTF8(), "Wave", {"Sine", "Triangle", "Square", "Saw", "Random"});
            id = prefix + "Sync";
            b(id.toRawUTF8(), "Sync", false);
            id = prefix + "Division";
            c(id.toRawUTF8(), "Division", {"1/1", "1/2", "1/4", "1/8", "1/16", "1/8 D", "1/8 T"}, 2);
            id = prefix + "Retrigger";
            b(id.toRawUTF8(), "Retrigger", true);
        }
        for (int n = 0; n < 8; ++n)
        {
            auto prefix = "mod" + juce::String(n);
            auto id = prefix + "Source";
            c(id.toRawUTF8(), "Source",
              {"Off", "LFO 1", "LFO 2", "Amp Env", "Filter Env", "Pitch Env", "Velocity", "Note", "Random",
               "Aftertouch", "Mod Wheel"});
            id = prefix + "Dest";
            c(id.toRawUTF8(), "Destination", {"Pitch", "Cutoff", "Volume", "Pan", "Osc Level", "Drive"});
            id = prefix + "Amount";
            f(id.toRawUTF8(), "Amount", -1, 1, 0);
        }
        b("delayOn", "Delay", false);
        f("delayTime", "Time", 1, 2000, 300, "ms", .4f);
        f("delayFeedback", "Feedback", 0, .95f, .35f);
        b("delaySync", "Sync", false);
        c("delayDivision", "Division", {"1/1", "1/2", "1/4", "1/8", "1/16", "1/8 D", "1/8 T"}, 3);
        f("delayHP", "High-pass", 20, 2000, 80, "Hz", .4f);
        f("delayLP", "Low-pass", 1000, 20000, 8000, "Hz", .4f);
        f("delayMix", "Mix", 0, 1, .25f);
        b("reverbOn", "Reverb", false);
        f("reverbSize", "Size", 0, 1, .5f);
        f("reverbDecay", "Decay", .1f, 15, 2, "s", .5f);
        f("reverbPre", "Pre-delay", 0, 250, 20, "ms", .5f);
        f("reverbDamping", "Damping", 0, 1, .5f);
        f("reverbMix", "Mix", 0, 1, .2f);
        b("chorusOn", "Chorus", false);
        f("chorusRate", "Rate", .01f, 10, .8f, "Hz", .4f);
        f("chorusDepth", "Depth", 0, 1, .3f);
        c("chorusVoices", "Voices", {"2", "4", "6"}, 1);
        f("chorusMix", "Mix", 0, 1, .25f);
        b("phaserOn", "Phaser", false);
        f("phaserRate", "Rate", .01f, 10, .4f, "Hz", .4f);
        f("phaserDepth", "Depth", 0, 1, .5f);
        f("phaserFeedback", "Feedback", -.95f, .95f, .2f);
        c("phaserStages", "Stages", {"2", "4", "6", "8", "12"}, 2);
        f("phaserMix", "Mix", 0, 1, .3f);
        b("saturationOn", "Saturation", false);
        c("saturationType", "Type", {"Tube", "Tape", "Transistor"});
        f("saturationDrive", "Drive", 0, 30, 6, "dB");
        f("saturationOutput", "Output", -30, 12, -3, "dB");
        f("saturationMix", "Mix", 0, 1, .5f);
        for (int n = 0; n < 5; ++n)
        {
            auto id = "fxOrder" + juce::String(n);
            c(id.toRawUTF8(), "Chain slot", {"Delay", "Reverb", "Chorus", "Phaser", "Saturation"}, n);
        }
        return p;
    }();
    return list;
}
APVTS::ParameterLayout makeParameters()
{
    APVTS::ParameterLayout layout;
    for (auto &s : specs())
    {
        juce::ParameterID id{s.id, 1};
        if (s.id == "root")
            layout.add(std::make_unique<juce::AudioParameterInt>(
                id, s.label, rootMinNote, rootMaxNote, rootReferenceNote,
                juce::AudioParameterIntAttributes()
                    .withStringFromValueFunction([](int note, int) { return rootNoteName(note); })
                    .withValueFromStringFunction([](const juce::String &text)
                                                 { return parseRootNoteName(text); })));
        else if (s.toggle)
            layout.add(std::make_unique<juce::AudioParameterBool>(id, s.label, s.initial > .5f));
        else if (!s.choices.isEmpty())
            layout.add(std::make_unique<juce::AudioParameterChoice>(id, s.label, s.choices, int(s.initial)));
        else
        {
            auto interval = (s.id == "root" || s.id == "polyphony" || s.id == "transpose" ||
                             s.id == "oscTune" || s.id == "bendRange")
                                ? 1.f
                                : 0.f;
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                id, s.label, juce::NormalisableRange<float>(s.min, s.max, interval, s.skew), s.initial,
                juce::AudioParameterFloatAttributes().withLabel(s.unit)));
        }
    }
    return layout;
}
} // namespace ra
