#include "SampleData.h"
namespace ra
{
std::shared_ptr<const SampleData> decodeSample(const void *bytes, size_t size, const juce::String &name,
                                               juce::String &error)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    auto stream = std::make_unique<juce::MemoryInputStream>(bytes, size, true);
    std::unique_ptr<juce::AudioFormatReader> reader(fm.createReaderFor(std::move(stream)));
    if (!reader)
    {
        error = "Unsupported or damaged audio file.";
        return {};
    }
    if (reader->lengthInSamples < 16 || reader->lengthInSamples > int64_t(reader->sampleRate * 600) ||
        reader->sampleRate < 8000 || reader->sampleRate > 384000)
    {
        error = "Use an audio file between 16 samples and 10 minutes (8–384 kHz).";
        return {};
    }
    auto s = std::make_shared<SampleData>();
    s->rate = reader->sampleRate;
    s->name = name;
    s->encoded.append(bytes, size);
    s->audio.setSize(2, int(reader->lengthInSamples));
    reader->read(&s->audio, 0, s->length(), 0, true, true);
    if (reader->numChannels == 1)
        s->audio.copyFrom(1, 0, s->audio, 0, 0, s->length());
    return s;
}
std::shared_ptr<const SampleData> makeInitSample()
{
    auto s = std::make_shared<SampleData>();
    s->audio.setSize(2, 22050);
    juce::Random random(761);
    float last = 0;
    for (int i = 0; i < s->length(); ++i)
    {
        float noise = random.nextFloat() * 2 - 1;
        float high = noise - last;
        last = noise;
        float t = float(i) / 44100;
        float value = .27f * high * std::exp(-t * 24.f) * juce::jmin(1.f, t * 3000.f);
        s->audio.setSample(0, i, value);
        s->audio.setSample(1, i, value);
    }
    s->name = "Hat_Closed (factory)";
    return s;
}
// Offline waveform-similarity overlap-add. Shared offsets preserve stereo phase.
std::shared_ptr<const SampleData> stretchSample(const SampleData &input, double factor)
{
    if (std::abs(factor - 1) < .001)
        return std::make_shared<SampleData>(input);
    auto out = std::make_shared<SampleData>();
    out->name = input.name;
    out->rate = input.rate;
    int length = int(juce::jlimit(16., input.rate * 600., input.length() * factor));
    out->audio.setSize(2, length);
    out->audio.clear();
    int window = juce::jmin(2048, input.length() / 2);
    window = juce::jmax(16, window);
    int hop = window / 4, search = juce::jmin(256, window / 4);
    std::vector<float> weight(size_t(length), 0.f);
    int previous = 0;
    for (int dst = 0; dst < length; dst += hop)
    {
        int expected = juce::jlimit(0, juce::jmax(0, input.length() - window), int(dst / factor));
        int chosen = expected;
        if (dst > 0 && expected + window < input.length())
        {
            double best = -1e30;
            for (int off = juce::jmax(0, expected - search);
                 off <= juce::jmin(input.length() - window, expected + search); off += 4)
            {
                double dot = 0, energy = 1e-12, ref = 1e-12;
                for (int j = 0; j < window - hop; j += 8)
                {
                    float a = input.audio.getSample(0, juce::jmin(input.length() - 1, previous + hop + j)),
                          b = input.audio.getSample(0, off + j);
                    dot += a * b;
                    energy += b * b;
                    ref += a * a;
                }
                double score = dot / std::sqrt(energy * ref);
                if (score > best)
                {
                    best = score;
                    chosen = off;
                }
            }
        }
        for (int j = 0; j < window && dst + j < length; ++j)
        {
            int src = juce::jmin(input.length() - 1, chosen + j);
            float w = .5f - .5f * std::cos(juce::MathConstants<float>::twoPi * float(j + .5f) / window);
            weight[size_t(dst + j)] += w;
            for (int ch = 0; ch < 2; ++ch)
                out->audio.addSample(ch, dst + j, input.audio.getSample(ch, src) * w);
        }
        previous = chosen;
    }
    for (int i = 0; i < length; ++i)
        if (weight[size_t(i)] > .00001f)
            for (int c = 0; c < 2; ++c)
                out->audio.setSample(c, i, out->audio.getSample(c, i) / weight[size_t(i)]);
    return out;
}
float snapToTransient(const SampleData &s, float fraction)
{
    int at = juce::jlimit(0, s.length() - 1, int(fraction * (s.length() - 1)));
    int radius = int(s.rate * .01);
    float best = -1;
    int found = at;
    for (int i = juce::jmax(1, at - radius); i < juce::jmin(s.length() - 1, at + radius); ++i)
    {
        float a = std::abs(s.audio.getSample(0, i)), before = std::abs(s.audio.getSample(0, i - 1));
        float score = a - before;
        if (score > best)
        {
            best = score;
            found = i;
        }
    }
    return float(found) / float(s.length() - 1);
}
} // namespace ra
