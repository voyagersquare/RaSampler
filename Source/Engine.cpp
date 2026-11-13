#include "Engine.h"
namespace ra
{
void Biquad::configure(int type, double sr, double freq, double q, double gain)
{
    freq = juce::jlimit(15., sr * .45, freq);
    q = juce::jlimit(.1, 10., q);
    double w = juce::MathConstants<double>::twoPi * freq / sr, c = std::cos(w), s = std::sin(w),
           alpha = s / (2 * q), a0 = 1 + alpha;
    if (type == 0 || type == 1)
    {
        b0 = (1 - c) / 2;
        b1 = 1 - c;
        b2 = b0;
        a1 = -2 * c;
        a2 = 1 - alpha;
    }
    else if (type == 2 || type == 3)
    {
        b0 = (1 + c) / 2;
        b1 = -(1 + c);
        b2 = b0;
        a1 = -2 * c;
        a2 = 1 - alpha;
    }
    else if (type == 4)
    {
        b0 = alpha;
        b1 = 0;
        b2 = -alpha;
        a1 = -2 * c;
        a2 = 1 - alpha;
    }
    else if (type == 5)
    {
        b0 = 1;
        b1 = -2 * c;
        b2 = 1;
        a1 = -2 * c;
        a2 = 1 - alpha;
    }
    else
    {
        double a = std::pow(10., gain / 40);
        b0 = 1 + alpha * a;
        b1 = -2 * c;
        b2 = 1 - alpha * a;
        a0 = 1 + alpha / a;
        a1 = -2 * c;
        a2 = 1 - alpha / a;
    }
    b0 /= a0;
    b1 /= a0;
    b2 /= a0;
    a1 /= a0;
    a2 /= a0;
}
Engine::Engine(APVTS &s) : state(s)
{
    for (auto &spec : specs())
    {
        parameters[spec.id] = state.getRawParameterValue(spec.id);
        values[spec.id] = spec.initial;
    }
}
void Engine::prepare(double sampleRate, int block)
{
    sr = sampleRate;
    delay.setSize(2, int(sr * 12) + 4);
    preDelay.setSize(2, int(sr * .3) + 4);
    chorusDry.setSize(2, juce::jmax(8192, block));
    chorusWet.setSize(2, juce::jmax(8192, block));
    for (auto &c : chorus)
        c.prepare({sr, juce::uint32(juce::jmax(8192, block)), 2});
    reverb.setSampleRate(sr);
    for (auto &v : voices)
    {
        v.amp.setSampleRate(sr);
        v.filter.setSampleRate(sr);
        v.pitchEnv.setSampleRate(sr);
    }
    reset();
}
void Engine::reset()
{
    for (auto &v : voices)
    {
        v.active = false;
        v.sample.reset();
        v.amp.reset();
        v.filter.reset();
        v.pitchEnv.reset();
    }
    delay.clear();
    preDelay.clear();
    delayPos = prePos = 0;
    reverb.reset();
    for (auto &c : chorus)
        c.reset();
    for (auto &ch : phaserMemory)
        for (auto &x : ch)
            x = 0;
    for (int i = 0; i < 2; ++i)
    {
        hpX[i] = hpY[i] = lpY[i] = phaserFeedback[i] = 0;
    }
    bends.fill(0);
    aftertouch.fill(0);
    wheel.fill(0);
    sustain.fill(false);
    voicesActive = 0;
}
static juce::ADSR::Parameters env(float a, float d, float s, float r)
{
    return {a * .001f, d * .001f, s, r * .001f};
}
void Engine::event(const juce::MidiMessage &m, std::shared_ptr<const SampleData> sample)
{
    int ch = juce::jlimit(0, 15, m.getChannel() - 1);
    if (int(p("midiChannel")) != 0 && int(p("midiChannel")) != ch + 1)
        return;
    if (m.isNoteOn())
    {
        int count = juce::jlimit(1, 32, int(p("polyphony")));
        Voice *target = nullptr;
        for (int i = 0; i < count; ++i)
            if (!voices[size_t(i)].active)
            {
                target = &voices[size_t(i)];
                break;
            }
        if (!target)
        {
            target = &voices[0];
            for (int i = 1; i < count; ++i)
                if (voices[size_t(i)].age < target->age)
                    target = &voices[size_t(i)];
        }
        auto &v = *target;
        v.sample = std::move(sample);
        v.note = m.getNoteNumber();
        v.channel = ch + 1;
        v.velocity = std::pow(m.getFloatVelocity(), p("velocityCurve"));
        v.random = juce::Random::getSystemRandom().nextFloat() * 2 - 1;
        v.age = ++counter;
        v.active = true;
        v.held = true;
        v.pedal = false;
        v.pressure = -1;
        v.pos =
            p("reverse") > .5f ? p("end") * (v.sample->length() - 1) : p("start") * (v.sample->length() - 1);
        v.pitch = p("glide") > 0 ? lastNote : float(v.note);
        lastNote = float(v.note);
        v.oscPhase = 0;
        v.amp.reset();
        v.filter.reset();
        v.pitchEnv.reset();
        v.amp.setParameters(env(p("ampAttack"), p("ampDecay"), p("ampSustain"), p("ampRelease")));
        v.filter.setParameters(
            env(p("filterAttack"), p("filterDecay"), p("filterSustain"), p("filterRelease")));
        v.pitchEnv.setParameters(
            env(p("pitchAttack"), p("pitchDecay"), p("pitchSustain"), p("pitchRelease")));
        v.amp.noteOn();
        v.filter.noteOn();
        v.pitchEnv.noteOn();
        for (int n = 0; n < 2; ++n)
            if (p("lfo" + juce::String(n + 1) + "Retrigger") > .5f)
                v.lfoPhase[n] = 0;
        for (auto &chFilter : v.filters)
            for (auto &f : chFilter)
                f.reset();
    }
    else if (m.isNoteOff())
    {
        for (auto &v : voices)
            if (v.active && v.note == m.getNoteNumber() && v.channel == ch + 1)
            {
                v.held = false;
                if (p("playMode") < .5f)
                {
                    if (sustain[size_t(ch)] && p("sustainEnable") > .5f)
                        v.pedal = true;
                    else
                    {
                        v.amp.noteOff();
                        v.filter.noteOff();
                        v.pitchEnv.noteOff();
                    }
                }
            }
    }
    else if (m.isPitchWheel())
        bends[size_t(ch)] = (m.getPitchWheelValue() - 8192) / 8192.f;
    else if (m.isChannelPressure() && p("aftertouchEnable") > .5f)
        aftertouch[size_t(ch)] = m.getChannelPressureValue() / 127.f;
    else if (m.isAftertouch() && p("aftertouchEnable") > .5f)
    {
        for (auto &v : voices)
            if (v.channel == ch + 1 && v.note == m.getNoteNumber())
                v.pressure = m.getAfterTouchValue() / 127.f;
    }
    else if (m.isController())
    {
        int cc = m.getControllerNumber();
        if (cc == 1)
            wheel[size_t(ch)] = m.getControllerValue() / 127.f;
        if (cc == 64)
        {
            bool down = m.getControllerValue() >= 64 && p("sustainEnable") > .5f;
            sustain[size_t(ch)] = down;
            if (!down)
                for (auto &v : voices)
                    if (v.channel == ch + 1 && v.pedal)
                    {
                        v.pedal = false;
                        v.amp.noteOff();
                        v.filter.noteOff();
                        v.pitchEnv.noteOff();
                    }
        }
        if (cc == 120)
        {
            for (auto &v : voices)
                if (v.channel == ch + 1)
                {
                    v.active = false;
                    v.amp.reset();
                }
        }
        if (cc == 123)
        {
            for (auto &v : voices)
                if (v.channel == ch + 1)
                {
                    v.held = v.pedal = false;
                    v.amp.noteOff();
                    v.filter.noteOff();
                    v.pitchEnv.noteOff();
                }
        }
    }
}
static double division(int index)
{
    static const double d[] = {4, 2, 1, .5, .25, .75, 1. / 3};
    return d[juce::jlimit(0, 6, index)];
}
void Engine::process(juce::AudioBuffer<float> &b, juce::MidiBuffer &midi,
                     std::shared_ptr<const SampleData> sample, double tempo)
{
    juce::ScopedNoDenormals denorm;
    for (auto &item : parameters)
        values[item.first] = item.second->load();
    bpm = juce::jlimit(20., 400., tempo);
    b.clear();
    for (int i = juce::jlimit(1, 32, int(p("polyphony"))); i < 32; ++i)
        voices[size_t(i)].active = false;
    int at = 0;
    for (auto metadata : midi)
    {
        int next = juce::jlimit(at, b.getNumSamples(), metadata.samplePosition);
        render(b, at, next - at);
        event(metadata.getMessage(), sample);
        at = next;
    }
    render(b, at, b.getNumSamples() - at);
    effects(b);
    peak = 0;
    for (int ch = 0; ch < b.getNumChannels(); ++ch)
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            float x = b.getSample(ch, i);
            if (!std::isfinite(x))
                x = 0;
            b.setSample(ch, i, x);
            peak = juce::jmax(peak, std::abs(x));
        }
    }
    int count = 0;
    for (auto &v : voices)
        if (v.active)
            ++count;
    voicesActive = count;
}
void Engine::render(juce::AudioBuffer<float> &buffer, int offset, int count)
{
    const float c_aftertouchEnable = p("aftertouchEnable");
    const float c_ampAttack = p("ampAttack");
    const float c_ampDecay = p("ampDecay");
    const float c_ampRelease = p("ampRelease");
    const float c_ampSustain = p("ampSustain");
    const float c_bendRange = p("bendRange");
    const float c_cutoff = p("cutoff");
    const float c_detune = p("detune");
    const float c_drive = p("drive");
    const float c_end = p("end");
    const float c_fadeIn = p("fadeIn");
    const float c_fadeOut = p("fadeOut");
    const float c_filterAmount = p("filterAmount");
    const float c_filterAttack = p("filterAttack");
    const float c_filterDecay = p("filterDecay");
    const float c_filterOn = p("filterOn");
    const float c_filterRelease = p("filterRelease");
    const float c_filterSustain = p("filterSustain");
    const float c_filterType = p("filterType");
    const float c_glide = p("glide");
    const float c_loop = p("loop");
    const float c_loopEnd = p("loopEnd");
    const float c_loopStart = p("loopStart");
    const float c_oscLevel = p("oscLevel");
    const float c_oscOn = p("oscOn");
    const float c_oscTune = p("oscTune");
    const float c_oscWave = p("oscWave");
    const float c_pan = p("pan");
    const float c_peqGain = p("peqGain");
    const float c_pitchAmount = p("pitchAmount");
    const float c_pitchAttack = p("pitchAttack");
    const float c_pitchDecay = p("pitchDecay");
    const float c_pitchRelease = p("pitchRelease");
    const float c_pitchSustain = p("pitchSustain");
    const float c_resonance = p("resonance");
    const float c_reverse = p("reverse");
    const float c_root = float(juce::jlimit(rootMinNote, rootMaxNote, juce::roundToInt(p("root"))));
    const float rootShift = c_root - rootReferenceNote;
    const float c_sampleGain = p("sampleGain");
    const float c_start = p("start");
    const float c_transpose = p("transpose");
    const float c_volume = p("volume");

    double lfoRate[2];
    int lfoWave[2];
    for (int n = 0; n < 2; ++n)
    {
        auto prefix = "lfo" + juce::String(n + 1);
        lfoRate[n] = p(prefix + "Sync") > .5f ? bpm / (60 * division(int(p(prefix + "Division"))))
                                              : p(prefix + "Rate");
        lfoWave[n] = int(p(prefix + "Wave"));
    }
    int modSource[8], modDest[8];
    float modAmount[8];
    for (int n = 0; n < 8; ++n)
    {
        auto id = "mod" + juce::String(n);
        modSource[n] = juce::jlimit(0, 10, int(p(id + "Source")));
        modDest[n] = juce::jlimit(0, 5, int(p(id + "Dest")));
        modAmount[n] = p(id + "Amount");
    }
    for (auto &v : voices)
    {
        if (!v.active || !v.sample)
            continue;
        auto &s = *v.sample;
        int begin = juce::jlimit(0, s.length() - 2, int(c_start * (s.length() - 1))),
            end = juce::jlimit(begin + 1, s.length() - 1, int(c_end * (s.length() - 1)));
        int loopA = juce::jlimit(begin, end - 1, int(c_loopStart * (s.length() - 1))),
            loopB = juce::jlimit(loopA + 1, end, int(c_loopEnd * (s.length() - 1)));
        bool reverse = c_reverse > .5f;
        v.amp.setParameters(env(c_ampAttack, c_ampDecay, c_ampSustain, c_ampRelease));
        v.filter.setParameters(env(c_filterAttack, c_filterDecay, c_filterSustain, c_filterRelease));
        v.pitchEnv.setParameters(env(c_pitchAttack, c_pitchDecay, c_pitchSustain, c_pitchRelease));
        for (int i = 0; i < count && v.active; ++i)
        {
            if (v.pos < begin || v.pos > end)
            {
                if (c_loop > .5f)
                    v.pos = reverse ? loopB : loopA;
                else
                {
                    v.active = false;
                    break;
                }
            }
            float amp = v.amp.getNextSample(), fenv = v.filter.getNextSample(),
                  penv = v.pitchEnv.getNextSample();
            if (!v.amp.isActive())
            {
                v.active = false;
                break;
            }
            float lfo[2];
            for (int n = 0; n < 2; ++n)
            {
                double rate = lfoRate[n];
                v.lfoPhase[n] += rate / sr;
                if (v.lfoPhase[n] >= 1)
                {
                    v.lfoPhase[n] -= 1;
                    v.randomLfo[n] = juce::Random::getSystemRandom().nextFloat() * 2 - 1;
                }
                double phase = v.lfoPhase[n];
                switch (lfoWave[n])
                {
                case 0:
                    lfo[n] = float(std::sin(phase * juce::MathConstants<double>::twoPi));
                    break;
                case 1:
                    lfo[n] = float(1 - 4 * std::abs(phase - .5));
                    break;
                case 2:
                    lfo[n] = phase < .5 ? 1.f : -1.f;
                    break;
                case 3:
                    lfo[n] = float(phase * 2 - 1);
                    break;
                default:
                    lfo[n] = v.randomLfo[n];
                }
            }
            float source[] = {0,
                              lfo[0],
                              lfo[1],
                              amp,
                              fenv,
                              penv,
                              v.velocity,
                              (v.note - 60) / 60.f,
                              v.random,
                              c_aftertouchEnable > .5f
                                  ? (v.pressure >= 0 ? v.pressure : aftertouch[size_t(v.channel - 1)])
                                  : 0,
                              wheel[size_t(v.channel - 1)]};
            float mod[6]{};
            for (int row = 0; row < 8; ++row)
                mod[modDest[row]] += source[modSource[row]] * modAmount[row];
            if (c_glide > 0)
                v.pitch += (v.note - v.pitch) * (1 - std::exp(-1 / (c_glide * .001 * sr)));
            else
                v.pitch = v.note;
            // Root Note is an ascending base-pitch selector; C3 is neutral for the original sample.
            double semitones = v.pitch - rootReferenceNote + rootShift + c_transpose + c_detune * .01 +
                               bends[size_t(v.channel - 1)] * c_bendRange + penv * c_pitchAmount +
                               mod[0] * 24;
            double increment = s.rate / sr * std::pow(2., semitones / 12);
            int index = juce::jlimit(0, s.length() - 2, int(v.pos));
            float frac = float(v.pos - index);
            float fade = 1;
            double elapsed = reverse ? end - v.pos : v.pos - begin,
                   remaining = reverse ? v.pos - begin : end - v.pos;
            if (c_fadeIn > 0)
                fade *= float(juce::jlimit(0., 1., elapsed / (s.rate * c_fadeIn * .001)));
            if (c_fadeOut > 0)
                fade *= float(juce::jlimit(0., 1., remaining / (s.rate * c_fadeOut * .001)));
            double freq = 440 * std::pow(2., (v.pitch + rootShift + c_transpose + c_oscTune - 69 +
                                              c_detune * .01 + bends[size_t(v.channel - 1)] * c_bendRange +
                                              penv * c_pitchAmount + mod[0] * 24) /
                                                 12);
            v.oscPhase += juce::jmin(.49, freq / sr);
            v.oscPhase -= std::floor(v.oscPhase);
            float osc = 0;
            if (c_oscOn > .5f)
            {
                switch (int(c_oscWave))
                {
                case 0:
                    osc = float(std::sin(v.oscPhase * juce::MathConstants<double>::twoPi));
                    break;
                case 1:
                    osc = v.oscPhase < .5 ? 1.f : -1.f;
                    break;
                case 2:
                    osc = float(v.oscPhase * 2 - 1);
                    break;
                default:
                    osc = juce::Random::getSystemRandom().nextFloat() * 2 - 1;
                }
                osc *= juce::Decibels::decibelsToGain(c_oscLevel + mod[4] * 24);
            }
            int type = int(c_filterType);
            if ((i & 15) == 0 && c_filterOn > .5f)
            {
                double cutoff = c_cutoff * std::pow(2., fenv * c_filterAmount + mod[1] * 6);
                for (auto &pair : v.filters)
                    for (auto &filter : pair)
                        filter.configure(type, sr, cutoff, c_resonance, c_peqGain);
            }
            float pan = juce::jlimit(-1.f, 1.f, c_pan + mod[3]),
                  gain = amp * v.velocity * juce::Decibels::decibelsToGain(c_volume + mod[2] * 24),
                  drive = juce::Decibels::decibelsToGain(juce::jlimit(0.f, 48.f, c_drive + mod[5] * 24));
            for (int ch = 0; ch < 2; ++ch)
            {
                float x =
                    (s.audio.getSample(ch, index) * (1 - frac) + s.audio.getSample(ch, index + 1) * frac) *
                        fade * juce::Decibels::decibelsToGain(c_sampleGain) +
                    osc;
                if (c_filterOn > .5f)
                {
                    if (drive > 1.001f)
                        x = std::tanh(x * drive) / std::sqrt(drive);
                    x = v.filters[ch][0].tick(x);
                    if (type == 1 || type == 3)
                        x = v.filters[ch][1].tick(x);
                }
                float panGain = ch == 0 ? std::sqrt(1 - pan) : std::sqrt(1 + pan);
                buffer.addSample(ch, offset + i, x * gain * panGain);
            }
            v.pos += (reverse ? -increment : increment);
            if (c_loop > .5f)
            {
                double length = loopB - loopA;
                if (!reverse && v.pos >= loopB)
                    v.pos = loopA + std::fmod(v.pos - loopA, length);
                if (reverse && v.pos < loopA)
                    v.pos = loopB - std::fmod(loopA - v.pos, length);
            }
        }
    }
}
void Engine::effects(juce::AudioBuffer<float> &b)
{
    const float c_chorusDepth = p("chorusDepth");
    const float c_chorusMix = p("chorusMix");
    const float c_chorusOn = p("chorusOn");
    const float c_chorusRate = p("chorusRate");
    const float c_chorusVoices = p("chorusVoices");
    const float c_delayDivision = p("delayDivision");
    const float c_delayFeedback = p("delayFeedback");
    const float c_delayHP = p("delayHP");
    const float c_delayLP = p("delayLP");
    const float c_delayMix = p("delayMix");
    const float c_delayOn = p("delayOn");
    const float c_delaySync = p("delaySync");
    const float c_delayTime = p("delayTime");
    const float c_phaserDepth = p("phaserDepth");
    const float c_phaserFeedback = p("phaserFeedback");
    const float c_phaserMix = p("phaserMix");
    const float c_phaserOn = p("phaserOn");
    const float c_phaserRate = p("phaserRate");
    const float c_phaserStages = p("phaserStages");
    const float c_reverbDamping = p("reverbDamping");
    const float c_reverbDecay = p("reverbDecay");
    const float c_reverbMix = p("reverbMix");
    const float c_reverbOn = p("reverbOn");
    const float c_reverbPre = p("reverbPre");
    const float c_reverbSize = p("reverbSize");
    const float c_saturationDrive = p("saturationDrive");
    const float c_saturationMix = p("saturationMix");
    const float c_saturationOn = p("saturationOn");
    const float c_saturationOutput = p("saturationOutput");
    const float c_saturationType = p("saturationType");

    bool seen[5]{};
    int n = b.getNumSamples();
    for (int slot = 0; slot < 5; ++slot)
    {
        int fx = juce::jlimit(0, 4, int(p("fxOrder" + juce::String(slot))));
        if (seen[fx])
            continue;
        seen[fx] = true;
        if (fx == 0 && c_delayOn > .5f)
        {
            double time = c_delaySync > .5f ? 60 / bpm * division(int(c_delayDivision)) : c_delayTime * .001;
            int samples = juce::jlimit(1, delay.getNumSamples() - 1, int(sr * time));
            float hp = std::exp(-juce::MathConstants<float>::twoPi * c_delayHP / float(sr)),
                  lp = 1 - std::exp(-juce::MathConstants<float>::twoPi * c_delayLP / float(sr));
            for (int i = 0; i < n; ++i)
            {
                int read = (delayPos + delay.getNumSamples() - samples) % delay.getNumSamples();
                for (int ch = 0; ch < 2; ++ch)
                {
                    float dry = b.getSample(ch, i), wet = delay.getSample(ch, read);
                    hpY[ch] = hp * (hpY[ch] + wet - hpX[ch]);
                    hpX[ch] = wet;
                    lpY[ch] += lp * (hpY[ch] - lpY[ch]);
                    delay.setSample(ch, delayPos, dry + lpY[ch] * c_delayFeedback);
                    b.setSample(ch, i, dry * (1 - c_delayMix) + lpY[ch] * c_delayMix);
                }
                delayPos = (delayPos + 1) % delay.getNumSamples();
            }
        }
        if (fx == 1 && c_reverbOn > .5f)
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = juce::jlimit(0.f, .99f, .15f + c_reverbSize * .35f + c_reverbDecay / 30);
            rp.damping = c_reverbDamping;
            rp.wetLevel = 1;
            rp.dryLevel = 0;
            rp.width = 1;
            reverb.setParameters(rp);
            for (int i = 0; i < n; ++i)
            {
                float l = b.getSample(0, i), r = b.getSample(1, i);
                int count = juce::jlimit(0, preDelay.getNumSamples() - 1, int(sr * c_reverbPre * .001));
                int read = (prePos + preDelay.getNumSamples() - count) % preDelay.getNumSamples();
                float wl = count ? preDelay.getSample(0, read) : l,
                      wr = count ? preDelay.getSample(1, read) : r;
                preDelay.setSample(0, prePos, l);
                preDelay.setSample(1, prePos, r);
                prePos = (prePos + 1) % preDelay.getNumSamples();
                reverb.processStereo(&wl, &wr, 1);
                b.setSample(0, i, l * (1 - c_reverbMix) + wl * c_reverbMix);
                b.setSample(1, i, r * (1 - c_reverbMix) + wr * c_reverbMix);
            }
        }
        if (fx == 2 && c_chorusOn > .5f)
        {
            int pairs = int(c_chorusVoices) + 1;
            for (int offset = 0; offset < n; offset += chorusDry.getNumSamples())
            {
                int size = juce::jmin(chorusDry.getNumSamples(), n - offset);
                for (int ch = 0; ch < 2; ++ch)
                    chorusDry.copyFrom(ch, 0, b, ch, offset, size);
                for (int ch = 0; ch < 2; ++ch)
                    b.applyGain(ch, offset, size, 1 - c_chorusMix);
                for (int k = 0; k < pairs; ++k)
                {
                    for (int ch = 0; ch < 2; ++ch)
                        chorusWet.copyFrom(ch, 0, chorusDry, ch, 0, size);
                    auto &c = chorus[size_t(k)];
                    c.setRate(juce::jmin(99.f, c_chorusRate * (1 + k * .17f)));
                    c.setDepth(c_chorusDepth);
                    c.setCentreDelay(7.f + k * 4.f);
                    c.setFeedback(.05f);
                    c.setMix(1);
                    auto block = juce::dsp::AudioBlock<float>(chorusWet).getSubBlock(0, size);
                    juce::dsp::ProcessContextReplacing<float> ctx(block);
                    c.process(ctx);
                    for (int ch = 0; ch < 2; ++ch)
                        b.addFrom(ch, offset, chorusWet, ch, 0, size, c_chorusMix / pairs);
                }
            }
        }
        if (fx == 3 && c_phaserOn > .5f)
        {
            static const int stages[] = {2, 4, 6, 8, 12};
            int count = stages[juce::jlimit(0, 4, int(c_phaserStages))];
            for (int i = 0; i < n; ++i)
            {
                phaserPhase += c_phaserRate / sr;
                phaserPhase -= std::floor(phaserPhase);
                for (int ch = 0; ch < 2; ++ch)
                {
                    float dry = b.getSample(ch, i), wet = dry + phaserFeedback[ch] * c_phaserFeedback;
                    double hz = juce::jlimit(30., sr * .4,
                                             500 * std::pow(2., std::sin((phaserPhase + ch * .125) *
                                                                         juce::MathConstants<double>::twoPi) *
                                                                    c_phaserDepth * 3));
                    float t = float(std::tan(juce::MathConstants<double>::pi * hz / sr)),
                          a = (1 - t) / (1 + t);
                    for (int j = 0; j < count; ++j)
                    {
                        float y = -a * wet + phaserMemory[ch][j];
                        phaserMemory[ch][j] = wet + a * y;
                        wet = y;
                    }
                    phaserFeedback[ch] = std::tanh(wet);
                    b.setSample(ch, i, dry * (1 - c_phaserMix) + wet * c_phaserMix);
                }
            }
        }
        if (fx == 4 && c_saturationOn > .5f)
        {
            float drive = juce::Decibels::decibelsToGain(c_saturationDrive),
                  gain = juce::Decibels::decibelsToGain(c_saturationOutput);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < n; ++i)
                {
                    float dry = b.getSample(ch, i), x = dry * drive, wet = 0;
                    switch (int(c_saturationType))
                    {
                    case 0:
                        wet = (std::tanh(x + .2f) - std::tanh(.2f)) * .9f;
                        break;
                    case 1:
                        wet = std::tanh(x * .8f) * 1.1f;
                        break;
                    default:
                        wet = x / (1 + std::abs(x));
                    }
                    b.setSample(ch, i, dry * (1 - c_saturationMix) + wet * gain * c_saturationMix);
                }
        }
    }
}
} // namespace ra
