#include "PluginEditor.h"
namespace
{
const juce::Colour orange(0xffff9d00), text(0xffeeeeed), muted(0xffa5a5a1), black(0xff0b0d0e);
void panel(juce::Graphics &g, juce::Rectangle<float> r, float radius = 10)
{
    g.setColour(juce::Colour(0xff030405));
    g.fillRoundedRectangle(r.translated(0, 2), radius);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff292c2d), r.getX(), r.getY(),
                                           juce::Colour(0xff151718), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, radius);
    g.setColour(juce::Colour(0xff737878));
    g.drawRoundedRectangle(r.reduced(.7f), radius, 1.3f);
    g.setColour(juce::Colour(0xff35393b));
    g.drawRoundedRectangle(r.reduced(3), radius - 2, 1);
}
void write(juce::Graphics &g, const juce::String &s, juce::Rectangle<int> r, float size = 16,
           juce::Colour color = text, int align = juce::Justification::centredLeft)
{
    g.setColour(color);
    g.setFont(juce::FontOptions(size));
    g.drawFittedText(s, r, align, 1);
}
void grid(juce::Graphics &g, juce::Rectangle<float> r)
{
    g.setColour(black);
    g.fillRoundedRectangle(r, 3);
    g.setColour(juce::Colour(0xff343738));
    for (int i = 1; i < 8; ++i)
        g.drawVerticalLine(int(r.getX() + i * r.getWidth() / 8), r.getY(), r.getBottom());
    for (int i = 1; i < 5; ++i)
        g.drawHorizontalLine(int(r.getY() + i * r.getHeight() / 5), r.getX(), r.getRight());
}
const juce::StringArray effectNames{"Delay", "Reverb", "Chorus", "Phaser", "Saturation"};
const juce::StringArray effectIDs{"delayOn", "reverbOn", "chorusOn", "phaserOn", "saturationOn"};
} // namespace
RaLook::RaLook()
{
    setColour(juce::Label::textColourId, text);
    setColour(juce::Slider::textBoxTextColourId, text);
    setColour(juce::Slider::textBoxBackgroundColourId, black);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff424647));
    setColour(juce::TextButton::textColourOffId, text);
    setColour(juce::TextButton::textColourOnId, black);
    setColour(juce::ComboBox::textColourId, text);
    setColour(juce::ComboBox::backgroundColourId, black);
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff202324));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, orange);
    setColour(juce::PopupMenu::highlightedTextColourId, black);
}
void RaLook::drawButtonText(juce::Graphics &g, juce::TextButton &b, bool over, bool down)
{
    if (b.getProperties().contains("loopModeIcon"))
    {
        int mode = int(b.getProperties()["loopModeIcon"]);
        auto r = b.getLocalBounds();
        float cx = r.getCentreX(), cy = r.getCentreY();
        auto color = b.getToggleState() ? black : text;
        g.setColour(color);
        if (mode == 0)
        {
            write(g, "OFF", r, 12, color, juce::Justification::centred);
            return;
        }
        auto arrow = [&](float x1, float x2, float y)
        {
            g.drawLine(x1, y, x2, y, 2);
            float sign = x2 > x1 ? 1.f : -1.f;
            juce::Path t;
            t.addTriangle(x2, y, x2 - 5 * sign, y - 4, x2 - 5 * sign, y + 4);
            g.fillPath(t);
        };
        if (mode == 1)
        {
            arrow(cx - 12, cx + 8, cy);
            g.drawLine(cx + 13, cy - 8, cx + 13, cy + 8, 2);
        }
        if (mode == 2)
        {
            arrow(cx - 13, cx + 13, cy - 5);
            juce::Path p;
            p.startNewSubPath(cx + 13, cy);
            p.lineTo(cx + 13, cy + 7);
            p.lineTo(cx - 13, cy + 7);
            p.lineTo(cx - 13, cy);
            g.strokePath(p, juce::PathStrokeType(1.5f));
        }
        if (mode == 3)
        {
            arrow(cx - 13, cx + 13, cy - 5);
            arrow(cx + 13, cx - 13, cy + 5);
        }
        return;
    }
    if (b.getHeight() < 45)
    {
        juce::LookAndFeel_V4::drawButtonText(g, b, over, down);
        return;
    }
    auto color = b.getToggleState() ? black : text;
    g.setColour(color);
    juce::Path p;
    float cx = 42, cy = b.getHeight() * .5f;
    auto name = b.getButtonText();
    if (name == "Effects")
    {
        for (int row = 0; row < 3; ++row)
        {
            float y = cy - 11 + row * 11;
            g.drawLine(cx - 17, y, cx + 17, y, 2.2f);
            g.setColour(b.getToggleState() ? orange : juce::Colour(0xff303435));
            g.fillEllipse(cx - 10 + row * 6, y - 4, 8, 8);
            g.setColour(color);
            g.drawEllipse(cx - 10 + row * 6, y - 4, 8, 8, 2);
        }
    }
    else if (name == "Sample")
    {
        p.startNewSubPath(cx - 20, cy);
        float heights[] = {3, 8, 4, 17, 6, 22, 10, 4, 12, 3};
        for (int j = 0; j < 10; ++j)
        {
            float x = cx - 18 + j * 4;
            p.lineTo(x, cy - heights[j]);
            p.lineTo(x + 2, cy + heights[j] * .7f);
        }
        p.lineTo(cx + 23, cy);
        g.strokePath(p, juce::PathStrokeType(2));
    }
    else if (name == "Pitch/Osc")
    {
        p.startNewSubPath(cx - 20, cy);
        for (int j = 0; j <= 40; ++j)
            p.lineTo(cx - 20 + j, cy - 14 * std::sin(float(j) * juce::MathConstants<float>::twoPi / 40));
        g.strokePath(p, juce::PathStrokeType(2.5f));
    }
    else if (name == "Filter/Global")
    {
        p.startNewSubPath(cx - 20, cy + 12);
        p.lineTo(cx - 11, cy - 12);
        p.lineTo(cx + 11, cy - 12);
        p.lineTo(cx + 20, cy + 12);
        g.strokePath(p, juce::PathStrokeType(2.5f));
    }
    else if (name == "Modulation")
    {
        g.drawLine(cx - 16, cy + 7, cx + 5, cy - 13, 2);
        g.drawLine(cx + 5, cy - 13, cx + 18, cy + 12, 2);
        g.drawLine(cx - 16, cy + 7, cx + 18, cy + 12, 2);
        for (auto pos : {juce::Point<float>(cx - 16, cy + 7), {cx + 5, cy - 13}, {cx + 18, cy + 12}})
        {
            g.setColour(juce::Colour(0xff303435));
            g.fillEllipse(pos.x - 5, pos.y - 5, 10, 10);
            g.setColour(color);
            g.drawEllipse(pos.x - 5, pos.y - 5, 10, 10, 2);
        }
    }
    else
    {
        g.drawEllipse(cx - 17, cy - 17, 34, 34, 2);
        for (int j = 0; j < 3; ++j)
            g.fillEllipse(cx - 10 + j * 8, cy - 2, 4, 4);
    }
    write(g, name, b.getLocalBounds().withTrimmedLeft(74).withTrimmedRight(8),
          name == "Filter/Global" ? 21.f : 23.f, color, juce::Justification::centred);
}
void RaLook::drawButtonBackground(juce::Graphics &g, juce::Button &b, const juce::Colour &, bool over,
                                  bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced(1);
    auto a = b.getToggleState() ? orange : juce::Colour(over ? 0xff45494b : 0xff343839);
    auto z = b.getToggleState() ? juce::Colour(0xffed7e00) : juce::Colour(0xff202223);
    if (down)
        a = a.darker(.2f);
    g.setGradientFill(juce::ColourGradient(a, 0, 0, z, 0, r.getBottom(), false));
    g.fillRoundedRectangle(r, 6);
    g.setColour(b.getToggleState() ? juce::Colour(0xffffd17b) : juce::Colour(0xff7c8182));
    g.drawRoundedRectangle(r, 6, 1);
    g.setColour(juce::Colours::black.withAlpha(.7f));
    g.drawRoundedRectangle(r.reduced(-.6f), 6, 1);
}
void RaLook::drawRotarySlider(juce::Graphics &g, int x, int y, int w, int h, float pos, float start,
                              float end, juce::Slider &)
{
    float size = float(juce::jmin(w, h)) - 7;
    auto r = juce::Rectangle<float>(size, size).withCentre({x + w * .5f, y + h * .5f});
    auto c = r.getCentre();
    float radius = size * .5f;
    g.setColour(juce::Colours::black);
    g.fillEllipse(r.translated(0, 2).expanded(1));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff55595a), r.getX(), r.getY(),
                                           juce::Colour(0xff111314), r.getRight(), r.getBottom(), false));
    g.fillEllipse(r);
    g.setColour(juce::Colour(0xff898d8d));
    g.drawEllipse(r.reduced(2), 1);
    juce::Path arc;
    arc.addCentredArc(c.x, c.y, radius + 1, radius + 1, 0, start, end, true);
    g.setColour(juce::Colour(0xff080909));
    g.strokePath(arc, juce::PathStrokeType(3));
    juce::Path active;
    float angle = start + pos * (end - start);
    active.addCentredArc(c.x, c.y, radius + 1, radius + 1, 0, start, angle, true);
    g.setColour(orange);
    g.strokePath(active,
                 juce::PathStrokeType(3, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(text);
    g.drawLine(c.x + std::sin(angle) * radius * .58f, c.y - std::cos(angle) * radius * .58f,
               c.x + std::sin(angle) * radius * .88f, c.y - std::cos(angle) * radius * .88f, 2);
}
void RaLook::drawToggleButton(juce::Graphics &g, juce::ToggleButton &b, bool over, bool)
{
    auto r = juce::Rectangle<float>(17, 17).withPosition(3, (b.getHeight() - 17) * .5f);
    g.setColour(b.getToggleState() ? orange : juce::Colour(0xff101213));
    g.fillRoundedRectangle(r, 3);
    g.setColour(over ? text : muted);
    g.drawRoundedRectangle(r, 3, 1);
    if (b.getToggleState())
    {
        juce::Path p;
        p.startNewSubPath(r.getX() + 3, r.getY() + 8);
        p.lineTo(r.getX() + 7, r.getY() + 12);
        p.lineTo(r.getX() + 14, r.getY() + 4);
        g.setColour(black);
        g.strokePath(p, juce::PathStrokeType(2));
    }
    write(g, b.getButtonText(), b.getLocalBounds().withTrimmedLeft(27), 15);
}
void RaLook::drawComboBox(juce::Graphics &g, int w, int h, bool, int, int, int, int, juce::ComboBox &)
{
    auto r = juce::Rectangle<float>(0, 0, float(w), float(h)).reduced(.5);
    g.setColour(black);
    g.fillRoundedRectangle(r, 4);
    g.setColour(juce::Colour(0xff777b7b));
    g.drawRoundedRectangle(r, 4, 1);
    juce::Path p;
    p.startNewSubPath(float(w - 18), h * .43f);
    p.lineTo(float(w - 13), h * .63f);
    p.lineTo(float(w - 8), h * .43f);
    g.setColour(text);
    g.strokePath(p, juce::PathStrokeType(1.8f));
}
RaControl::RaControl(RaSamplerProcessor &p, const juce::String &parameter, bool small)
    : id(parameter), compact(small), processor(p)
{
    const ra::ParamSpec *spec = nullptr;
    for (auto &s : ra::specs())
        if (s.id == id)
        {
            spec = &s;
            break;
        }
    jassert(spec);
    if (!spec)
        return;
    label.setText(spec->label, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::FontOptions(compact ? 12.f : 14.f));
    addAndMakeVisible(label);
    if (spec->toggle)
    {
        toggle.setButtonText(spec->label);
        addAndMakeVisible(toggle);
        label.setVisible(false);
        ba = std::make_unique<ra::APVTS::ButtonAttachment>(p.state, id, toggle);
    }
    else if (!spec->choices.isEmpty())
    {
        combo.addItemList(spec->choices, 1);
        addAndMakeVisible(combo);
        ca = std::make_unique<ra::APVTS::ComboBoxAttachment>(p.state, id, combo);
    }
    else
    {
        slider.setSliderStyle(compact ? juce::Slider::LinearBar : juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(compact ? juce::Slider::TextBoxLeft : juce::Slider::TextBoxBelow, false,
                               compact ? 84 : 80, 19);
        slider.setDoubleClickReturnValue(true, spec->initial);
        slider.setPopupDisplayEnabled(false, false, nullptr);
        slider.setTextValueSuffix(spec->unit.isEmpty() || spec->unit == "Hz" ? "" : " " + spec->unit);
        slider.setNumDecimalPlacesToDisplay((spec->id == "root" || spec->id == "polyphony" ||
                                             spec->id == "transpose" || spec->id == "oscTune" ||
                                             spec->id == "bendRange")
                                                ? 0
                                                : 2);
        addAndMakeVisible(slider);
        sa = std::make_unique<ra::APVTS::SliderAttachment>(p.state, id, slider);
        slider.textFromValueFunction = [parameter, unit = spec->unit](double value)
        {
            if (parameter == "root")
                return ra::rootNoteName(juce::roundToInt(value));
            if (parameter == "pan")
                return std::abs(value) < .01
                           ? juce::String("C")
                           : juce::String(int(std::abs(value) * 100)) + (value < 0 ? " L" : " R");
            if (unit == "Hz")
                return value >= 1000 ? juce::String(value / 1000, 1) + " kHz"
                                     : juce::String(value, 1) + " Hz";
            bool integer = parameter == "transpose" || parameter == "polyphony" || parameter == "bendRange" ||
                           parameter == "oscTune";
            return juce::String(value, integer ? 0 : 2);
        };
        if (id == "root")
        {
            slider.valueFromTextFunction = [](const juce::String &text)
            { return double(ra::parseRootNoteName(text)); };
            slider.setScrollWheelEnabled(false);
            slider.setMouseDragSensitivity(360);
            slider.setTooltip("Root Note C1-C4. Each step raises sample pitch by one semitone; C3 is "
                              "neutral. Yellow keys show the selected note.");
            slider.onValueChange = [this]
            {
                if (auto *owner = findParentComponentOfClass<RaSamplerEditor>())
                    owner->rootNoteChanged(true);
            };
        }
        if (id == "pan")
            slider.valueFromTextFunction = [](const juce::String &value)
            {
                auto s = value.trim();
                if (s.equalsIgnoreCase("C"))
                    return 0.;
                if (s.endsWithIgnoreCase("L"))
                    return -std::abs(s.getDoubleValue()) / 100.;
                if (s.endsWithIgnoreCase("R"))
                    return std::abs(s.getDoubleValue()) / 100.;
                return s.getDoubleValue();
            };
        if (spec->unit == "Hz")
            slider.valueFromTextFunction = [](const juce::String &value)
            { return value.getDoubleValue() * (value.containsIgnoreCase("k") ? 1000. : 1.); };
        slider.updateText();
        slider.onDragStart = [&p, parameter]
        { p.setStatus("Editing " + parameter + " | double-click restores default"); };
    }
}
void RaControl::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel)
{
    if (id == "root" && isEnabled())
    {
        if (wheel.isInertial)
            return;
        float delta = wheel.deltaY != 0 ? wheel.deltaY : wheel.deltaX;
        if (delta != 0)
            processor.setParameter(
                "root", juce::jlimit(ra::rootMinNote, ra::rootMaxNote,
                                     juce::roundToInt(processor.value("root")) + (delta > 0 ? 1 : -1)));
        return;
    }
    juce::Component::mouseWheelMove(event, wheel);
}
void RaControl::resized()
{
    auto r = getLocalBounds();
    if (toggle.isVisible())
    {
        toggle.setBounds(r.reduced(2));
        return;
    }
    if (label.isVisible())
        label.setBounds(r.removeFromTop(compact ? 15 : 19));
    if (combo.isVisible())
        combo.setBounds(r.reduced(4, juce::jmax(1, (r.getHeight() - 27) / 2)));
    else
        slider.setBounds(r.reduced(compact ? 3 : 4, 0));
}
RaLoopModes::RaLoopModes(RaSamplerProcessor &p) : processor(p)
{
    const char *names[] = {"One-shot", "Forward", "Ping-pong"};
    const char *descriptions[] = {"Play once in the direction selected by Reverse, without repeating.",
                                  "Repeat LOOP IN to LOOP OUT in the direction selected by Reverse.",
                                  "Alternate between LOOP IN and LOOP OUT (ping-pong)."};
    for (int i = 0; i < 3; ++i)
    {
        auto &b = sustain[i];
        addAndMakeVisible(b);
        b.setButtonText(names[i]);
        b.setName("Sustain / " + juce::String(names[i]));
        b.getProperties().set("loopModeIcon", i + 1);
        b.setTooltip("Sustain: " + juce::String(descriptions[i]));
        b.onClick = [this, i]
        {
            processor.setParameter("loopSustainMode", float(i));
            if (processor.value("loopLink") > .5f)
                processor.setParameter("loopReleaseMode", float(i + 1));
            refresh();
        };
    }
    for (int i = 0; i < 4; ++i)
    {
        auto &b = release[i];
        addAndMakeVisible(b);
        auto name = i == 0 ? juce::String("Off") : juce::String(names[i - 1]);
        b.setButtonText(name);
        b.setName("Release / " + name);
        b.getProperties().set("loopModeIcon", i);
        b.setTooltip(
            i == 0
                ? "Release OFF: stop the voice immediately on Note Off (after the sustain pedal is lifted)."
                : "After Note Off: " + juce::String(descriptions[i - 1]) +
                      " The amplitude ADSR Release still controls fade-out time.");
        b.onClick = [this, i]
        {
            if (i == 0)
                processor.setParameter("loopLink", 0);
            else if (processor.value("loopLink") > .5f)
                processor.setParameter("loopSustainMode", float(i - 1));
            processor.setParameter("loopReleaseMode", float(i));
            refresh();
        };
    }
    addAndMakeVisible(link);
    link.setName("Link Loop Modes");
    link.setTooltip("Link Release Mode to Sustain Mode. Both use the existing LOOP IN / LOOP OUT markers. "
                    "Release OFF unlinks the modes.");
    link.onClick = [this]
    {
        bool linked = processor.value("loopLink") > .5f;
        int effective = ra::effectiveReleaseMode(int(processor.value("loopSustainMode")),
                                                 int(processor.value("loopReleaseMode")), linked);
        processor.setParameter("loopReleaseMode",
                               float(linked ? effective : int(processor.value("loopSustainMode")) + 1));
        processor.setParameter("loopLink", linked ? 0.f : 1.f);
        refresh();
    };
    refresh();
}
void RaLoopModes::resized()
{
    for (int i = 0; i < 3; ++i)
        sustain[i].setBounds(142 + i * 63, 0, 55, 32);
    for (int i = 0; i < 4; ++i)
        release[i].setBounds(142 + i * 63, 48, 55, 32);
    link.setBounds(350, 0, 110, 32);
}
void RaLoopModes::paint(juce::Graphics &g)
{
    bool enabled = processor.value("loop") > .5f, gate = processor.value("playMode") < .5f;
    write(g, "Sustain Mode", {0, 0, 134, 32}, 14, enabled ? text : muted);
    write(g, "Release Mode", {0, 48, 134, 32}, 14, enabled && gate ? text : muted);
    write(g,
          !enabled ? "Enable Loop to use these modes."
          : !gate  ? "Select Gate playback to use Release Mode."
                   : "Release follows Note Off and the amplitude ADSR.",
          {0, 91, getWidth(), 20}, 11, muted);
}
void RaLoopModes::refresh()
{
    bool enabled = processor.value("loop") > .5f, gate = processor.value("playMode") < .5f,
         linked = processor.value("loopLink") > .5f;
    int sm = juce::jlimit(0, 2, int(processor.value("loopSustainMode"))),
        rm = ra::effectiveReleaseMode(sm, int(processor.value("loopReleaseMode")), linked);
    for (int i = 0; i < 3; ++i)
    {
        sustain[i].setToggleState(i == sm, juce::dontSendNotification);
        sustain[i].setEnabled(enabled);
        sustain[i].setAlpha(enabled ? 1.f : .35f);
    }
    for (int i = 0; i < 4; ++i)
    {
        release[i].setToggleState(i == rm, juce::dontSendNotification);
        release[i].setEnabled(enabled && gate);
        release[i].setAlpha(enabled && gate ? 1.f : .35f);
    }
    link.setToggleState(linked, juce::dontSendNotification);
    link.setEnabled(enabled);
    link.setAlpha(enabled ? 1.f : .35f);
    repaint();
}
void RaWave::paint(juce::Graphics &g)
{
    auto r = getLocalBounds().toFloat();
    grid(g, r);
    auto s = processor.sample();
    if (s != cached || cachedLength != getWidth())
    {
        cached = s;
        cachedLength = getWidth();
        waveform.clear();
        float mid = r.getCentreY();
        waveform.startNewSubPath(0, mid);
        for (int x = 0; x < getWidth(); ++x)
        {
            int a = int(double(x) * s->length() / getWidth()),
                b = juce::jmin(s->length(), int(double(x + 1) * s->length() / getWidth()) + 1);
            float peak = 0;
            for (int i = a; i < b; ++i)
                peak = juce::jmax(peak, std::abs(s->audio.getSample(0, i)));
            waveform.lineTo(float(x), mid - juce::jmin(1.f, peak) * r.getHeight() * .47f);
        }
        for (int x = getWidth() - 1; x >= 0; --x)
        {
            int a = int(double(x) * s->length() / getWidth()),
                b = juce::jmin(s->length(), int(double(x + 1) * s->length() / getWidth()) + 1);
            float peak = 0;
            for (int i = a; i < b; ++i)
                peak = juce::jmax(peak, std::abs(s->audio.getSample(0, i)));
            waveform.lineTo(float(x), mid + juce::jmin(1.f, peak) * r.getHeight() * .47f);
        }
        waveform.closeSubPath();
    }
    g.setColour(orange);
    g.fillPath(waveform);
    g.drawHorizontalLine(getHeight() / 2, 0, float(getWidth()));
    float a = processor.value("start") * getWidth(), b = processor.value("end") * getWidth();
    g.setColour(juce::Colours::black.withAlpha(.65f));
    g.fillRect(0.f, 0.f, a, float(getHeight()));
    g.fillRect(b, 0.f, getWidth() - b, float(getHeight()));
    for (int n = 0; n < 2; ++n)
    {
        float x = n == 0 ? a : b;
        g.setColour(text);
        g.drawVerticalLine(juce::jlimit(0, getWidth() - 1, int(x)), 0, float(getHeight()));
        juce::Path t;
        t.addTriangle(x, float(n == 0 ? 0 : getHeight()), x + (n == 0 ? 10 : -10),
                      float(n == 0 ? 0 : getHeight()), x, float(n == 0 ? 12 : getHeight() - 12));
        g.fillPath(t);
    }
    paintEnvelope(g);
    paintLoopMarkers(g);
}
std::array<juce::Rectangle<float>, 2> RaWave::loopHandleBounds() const
{
    float width = float(getWidth()), height = float(getHeight());
    float start = juce::jlimit(0.f, 1.f, processor.value("loopStart")) * width;
    float end = juce::jlimit(0.f, 1.f, processor.value("loopEnd")) * width;
    auto inside = [width](float x, float w)
    { return juce::jlimit(12.f, juce::jmax(12.f, width - w - 12), x); };
    std::array<juce::Rectangle<float>, 2> handles = {
        juce::Rectangle<float>(inside(start + 8, 64), height - 17, 64, 15),
        juce::Rectangle<float>(inside(end - 80, 72), height - 17, 72, 15)};
    if (handles[0].expanded(3, 0).intersects(handles[1]))
        handles[0].translate(0, -18);
    return handles;
}
int RaWave::loopHit(juce::Point<float> point) const
{
    if (processor.value("loop") < .5f)
        return -1;
    auto handles = loopHandleBounds();
    for (int n = 0; n < 2; ++n)
        if (handles[size_t(n)].expanded(3).contains(point))
            return n;
    return -1;
}
void RaWave::paintLoopMarkers(juce::Graphics &g)
{
    if (processor.value("loop") < .5f)
        return;
    const juce::Colour cyan(0xff67e8ff);
    auto handles = loopHandleBounds();
    float start = juce::jlimit(0.f, float(getWidth() - 1), processor.value("loopStart") * getWidth());
    float end = juce::jlimit(start, float(getWidth() - 1), processor.value("loopEnd") * getWidth());
    g.setColour(cyan.withAlpha(.055f));
    g.fillRect(start, 0.f, end - start, float(getHeight()));
    for (int n = 0; n < 2; ++n)
    {
        float x = n == 0 ? start : end;
        auto box = handles[size_t(n)];
        // Separate the loop flags from the outer white trim handles, even for a full-sample loop.
        float top = 15, bottom = float(getHeight() - 19);
        g.setColour(black);
        g.drawLine(x, top, x, bottom, 5);
        g.setColour(cyan);
        g.drawLine(x, top, x, bottom, 2.5f);
        g.drawLine(x, top, x + (n == 0 ? 9 : -9), top, 2.5f);
        float attach = n == 0 ? box.getX() + 7 : box.getRight() - 7;
        g.drawLine(x, bottom, attach, box.getY(), 1.5f);
        g.setColour(black);
        g.fillRoundedRectangle(box.expanded(1.5f), 3);
        g.setColour(cyan);
        g.fillRoundedRectangle(box, 2.5f);
        write(g, n == 0 ? "LOOP IN" : "LOOP OUT", box.toNearestInt(), 10, black,
              juce::Justification::centred);
    }
}
RaWave::EnvelopeGeometry RaWave::envelopeGeometry() const
{
    EnvelopeGeometry shape;
    float left = juce::jmax(9.f, processor.value("start") * getWidth());
    float right = juce::jmin(float(getWidth() - 9), processor.value("end") * getWidth());
    if (right - left < 100)
    {
        left = 9;
        right = float(getWidth() - 9);
    }
    shape.bounds = {left, 20.f, juce::jmax(1.f, right - left), juce::jmax(1.f, float(getHeight() - 34))};
    auto sample = processor.sample();
    float selectedMs = float(1000. * (sample->length() - 1) / sample->rate) *
                       juce::jmax(0.f, processor.value("end") - processor.value("start"));
    float attack = processor.value("ampAttack"), decay = processor.value("ampDecay"),
          release = processor.value("ampRelease");
    float hold = juce::jmax(20.f, (attack + decay + release) * .12f);
    shape.previewMs = juce::jmax(selectedMs, attack + decay + release + hold);
    float width = shape.bounds.getWidth(), top = shape.bounds.getY(), bottom = shape.bounds.getBottom();
    float sustain = bottom - processor.value("ampSustain") * shape.bounds.getHeight();
    float a = left + attack / shape.previewMs * width;
    float d = left + (attack + decay) / shape.previewMs * width;
    float releaseStart = right - release / shape.previewMs * width;
    // Keep short stages selectable. Editing uses deltas, so clicking a node never changes its value.
    a = juce::jlimit(left + 2, right - 50, a);
    d = juce::jlimit(a + 12, right - 38, d);
    releaseStart = juce::jlimit(d + 12, right - 24, releaseStart);
    shape.handles = {juce::Point<float>(a, top),
                     {d, sustain},
                     {releaseStart, sustain},
                     {(releaseStart + right) * .5f, (sustain + bottom) * .5f}};
    return shape;
}
std::array<juce::Point<float>, 4> RaWave::envelopeHandles() const
{
    return envelopeGeometry().handles;
}
int RaWave::envelopeHit(juce::Point<float> point) const
{
    if (!processor.waveformEnvelope.load())
        return -1;
    auto handles = envelopeHandles();
    float closest = 10;
    int hit = -1;
    for (int n = 0; n < 4; ++n)
    {
        float distance = point.getDistanceFrom(handles[size_t(n)]);
        if (distance < closest)
        {
            closest = distance;
            hit = n;
        }
    }
    return hit;
}
void RaWave::paintEnvelope(juce::Graphics &g)
{
    if (!processor.waveformEnvelope.load())
        return;
    auto shape = envelopeGeometry();
    auto &h = shape.handles;
    juce::Path line;
    line.startNewSubPath(shape.bounds.getX(), shape.bounds.getBottom());
    line.lineTo(h[0]);
    line.lineTo(h[1]);
    line.lineTo(h[2]);
    line.lineTo(shape.bounds.getRight(), shape.bounds.getBottom());
    auto fill = line;
    fill.closeSubPath();
    g.setColour(juce::Colours::white.withAlpha(.045f));
    g.fillPath(fill);
    g.setColour(juce::Colours::black.withAlpha(.7f));
    g.strokePath(line, juce::PathStrokeType(3.6f));
    g.setColour(text.withAlpha(.95f));
    g.strokePath(line, juce::PathStrokeType(1.5f));
    static const char *names[] = {"A", "D", "S", "R"};
    for (int i = 0; i < 4; ++i)
    {
        auto point = h[size_t(i)];
        auto box = juce::Rectangle<float>(10, 10).withCentre(point);
        g.setColour(black);
        g.fillRoundedRectangle(box.expanded(1), 2);
        g.setColour(orange);
        g.fillRoundedRectangle(box, 2);
        write(g, names[i], box.toNearestInt(), 8, black, juce::Justification::centred);
    }
    write(g, "ADSR | " + juce::String(shape.previewMs / 1000.f, 2) + " s preview",
          {getWidth() - 179, 2, 174, 15}, 10, muted, juce::Justification::centredRight);
}
void RaWave::mouseMove(const juce::MouseEvent &e)
{
    int loop = loopHit(e.position);
    if (loop >= 0)
    {
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        setTooltip(loop == 0 ? "LOOP IN: drag to move the start of the loop"
                             : "LOOP OUT: drag to move the end of the loop");
        return;
    }
    int hit = envelopeHit(e.position);
    setMouseCursor(hit >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    static const char *ids[] = {"ampAttack", "ampDecay", "ampSustain", "ampRelease"};
    static const char *names[] = {"Attack", "Decay / Sustain", "Sustain", "Release"};
    setTooltip(hit >= 0 ? juce::String(names[hit]) + ": " + juce::String(processor.value(ids[hit]), 2) +
                              (hit == 2 ? "" : " ms") +
                              ". Drag to edit. Release begins at MIDI Note Off in Gate mode."
                        : "Drag sample boundaries or the cyan LOOP IN / LOOP OUT flags. Shift-drag anywhere "
                          "to edit the loop. ADSR preview fits all stages; "
                          "sustain duration follows MIDI Note Off.");
}
void RaWave::mouseDown(const juce::MouseEvent &e)
{
    int loopHandle = loopHit(e.position);
    if (!e.mods.isShiftDown() && loopHandle < 0 && (envelopeDrag = envelopeHit(e.position)) >= 0)
    {
        auto shape = envelopeGeometry();
        envelopeMouseDown = e.position;
        dragPreviewMs = shape.previewMs;
        dragWidth = shape.bounds.getWidth();
        dragHeight = shape.bounds.getHeight();
        initialAttack = processor.value("ampAttack");
        initialDecay = processor.value("ampDecay");
        initialSustain = processor.value("ampSustain");
        initialRelease = processor.value("ampRelease");
        envelopeParameters.clear();
        if (envelopeDrag == 0)
            envelopeParameters.push_back("ampAttack");
        if (envelopeDrag == 1)
        {
            envelopeParameters.push_back("ampDecay");
            envelopeParameters.push_back("ampSustain");
        }
        if (envelopeDrag == 2)
            envelopeParameters.push_back("ampSustain");
        if (envelopeDrag == 3)
            envelopeParameters.push_back("ampRelease");
        for (auto &id : envelopeParameters)
            processor.state.getParameter(id)->beginChangeGesture();
        return;
    }
    envelopeDrag = -1;
    float x = e.position.x / getWidth();
    if (loopHandle >= 0)
        dragging = loopHandle == 0 ? "loopStart" : "loopEnd";
    else if (e.mods.isShiftDown())
        dragging = std::abs(x - processor.value("loopStart")) < std::abs(x - processor.value("loopEnd"))
                       ? "loopStart"
                       : "loopEnd";
    else
        dragging =
            std::abs(x - processor.value("start")) < std::abs(x - processor.value("end")) ? "start" : "end";
    loopDragOffset = loopHandle >= 0 ? processor.value(dragging) - x : 0.f;
    if (auto *p = processor.state.getParameter(dragging))
        p->beginChangeGesture();
    if (loopHandle < 0)
        mouseDrag(e);
}
void RaWave::mouseDrag(const juce::MouseEvent &e)
{
    if (envelopeDrag >= 0)
    {
        float dx = e.position.x - envelopeMouseDown.x, dy = e.position.y - envelopeMouseDown.y;
        auto set = [this](const char *id, float value)
        {
            auto *p = processor.state.getParameter(id);
            p->setValueNotifyingHost(p->convertTo0to1(p->getNormalisableRange().snapToLegalValue(value)));
        };
        if (envelopeDrag == 0)
            set("ampAttack", initialAttack + dx * dragPreviewMs / dragWidth);
        if (envelopeDrag == 1)
        {
            set("ampDecay", initialDecay + dx * dragPreviewMs / dragWidth);
            set("ampSustain", initialSustain - dy / dragHeight);
        }
        if (envelopeDrag == 2)
            set("ampSustain", initialSustain - dy / dragHeight);
        if (envelopeDrag == 3)
            set("ampRelease", initialRelease - 2 * dx * dragPreviewMs / dragWidth);
        repaint();
        return;
    }
    if (dragging.isEmpty())
        return;
    float value = juce::jlimit(0.f, 1.f, e.position.x / getWidth() + loopDragOffset);
    if (processor.value("snap") > .5f)
        value = ra::snapToTransient(*processor.sample(), value);
    float gap = 1.f / processor.sample()->length();
    if (dragging == "start")
        value = juce::jmin(value, processor.value("end") - gap);
    if (dragging == "end")
        value = juce::jmax(value, processor.value("start") + gap);
    if (dragging == "loopStart")
        value = juce::jlimit(processor.value("start"),
                             juce::jmax(processor.value("start"), processor.value("loopEnd") - gap), value);
    if (dragging == "loopEnd")
        value = juce::jlimit(juce::jmin(processor.value("end"), processor.value("loopStart") + gap),
                             processor.value("end"), value);
    if (auto *p = processor.state.getParameter(dragging))
        p->setValueNotifyingHost(p->convertTo0to1(value));
    repaint();
}
void RaWave::mouseUp(const juce::MouseEvent &)
{
    if (envelopeDrag >= 0)
    {
        for (auto &id : envelopeParameters)
            processor.state.getParameter(id)->endChangeGesture();
        envelopeParameters.clear();
        envelopeDrag = -1;
        return;
    }
    if (auto *p = processor.state.getParameter(dragging))
        p->endChangeGesture();
    dragging.clear();
}
void RaFilterGraph::paint(juce::Graphics &g)
{
    auto r = getLocalBounds().toFloat();
    grid(g, r);
    ra::Biquad f;
    double rate = processor.getSampleRate() > 0 ? processor.getSampleRate() : 44100;
    int type = int(processor.value("filterType"));
    f.configure(type, rate, processor.value("cutoff"), processor.value("resonance"),
                processor.value("peqGain"));
    juce::Path line;
    for (int x = 0; x < getWidth(); ++x)
    {
        double hz = 20 * std::pow(1000., double(x) / getWidth()),
               w = juce::MathConstants<double>::twoPi * hz / rate;
        std::complex<double> z = std::exp(std::complex<double>(0, -w));
        double mag = std::abs((f.b0 + f.b1 * z + f.b2 * z * z) / (1. + f.a1 * z + f.a2 * z * z));
        if (type == 1 || type == 3)
            mag *= mag;
        if (processor.value("filterOn") < .5f)
            mag = 1;
        float y = juce::jlimit(
            1.f, float(getHeight() - 1),
            float(getHeight() * .35 - 20 * std::log10(juce::jmax(1e-6, mag)) * getHeight() / 72));
        if (x == 0)
            line.startNewSubPath(0, y);
        else
            line.lineTo(float(x), y);
    }
    g.setColour(orange);
    g.strokePath(line, juce::PathStrokeType(2));
    line.lineTo(float(getWidth()), float(getHeight()));
    line.lineTo(0, float(getHeight()));
    line.closeSubPath();
    g.setGradientFill(juce::ColourGradient(orange.withAlpha(.25f), 0, 0, orange.withAlpha(.02f), 0,
                                           float(getHeight()), false));
    g.fillPath(line);
}
void RaFilterGraph::mouseDrag(const juce::MouseEvent &e)
{
    processor.setParameter("cutoff",
                           float(20 * std::pow(1000., juce::jlimit(0.f, 1.f, e.position.x / getWidth()))));
    processor.setParameter("resonance", .1f + juce::jlimit(0.f, 1.f, 1 - e.position.y / getHeight()) * 4);
    repaint();
}
void RaVelocity::paint(juce::Graphics &g)
{
    auto r = getLocalBounds().toFloat();
    grid(g, r);
    juce::Path p;
    p.startNewSubPath(1, float(getHeight() - 1));
    for (int x = 1; x < getWidth(); ++x)
        p.lineTo(float(x), float(getHeight() - 1) *
                               (1 - std::pow(float(x) / (getWidth() - 1), processor.value("velocityCurve"))));
    g.setColour(orange);
    g.strokePath(p, juce::PathStrokeType(2));
}
void RaVelocity::mouseDrag(const juce::MouseEvent &e)
{
    float x = juce::jlimit(.05f, .95f, e.position.x / getWidth()),
          y = juce::jlimit(.01f, .99f, 1 - e.position.y / getHeight());
    processor.setParameter("velocityCurve", juce::jlimit(.2f, 4.f, std::log(y) / std::log(x)));
    repaint();
}
void RaKeyboard::drawWhiteNote(int note, juce::Graphics &g, juce::Rectangle<float> r, bool down, bool over,
                               juce::Colour, juce::Colour)
{
    bool root = note == juce::roundToInt(processor.value("root"));
    const juce::Colour yellow(0xffffd34a);
    auto top = root ? yellow : down ? orange : juce::Colour(0xfff0f0ed);
    auto bottom = root   ? juce::Colour(0xffdca51b)
                  : down ? juce::Colour(0xffdb7800)
                         : juce::Colour(0xffbabdbc);
    if (over || root && down)
        top = top.brighter(.12f);
    g.setGradientFill(juce::ColourGradient(top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false));
    g.fillRect(r.reduced(.5f));
    g.setColour(black);
    g.drawRect(r, 1);
    if (root)
    {
        g.setColour(yellow);
        g.drawRect(r.reduced(1.5f), 1.5f);
    }
    if (note % 12 == 0 || root)
        write(g, juce::MidiMessage::getMidiNoteName(note, true, true, 3),
              r.toNearestInt().withTrimmedTop(int(r.getHeight() - 23)), 12, black,
              juce::Justification::centred);
}
void RaKeyboard::drawBlackNote(int note, juce::Graphics &g, juce::Rectangle<float> r, bool down, bool over,
                               juce::Colour)
{
    bool root = note == juce::roundToInt(processor.value("root"));
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(r.translated(1, 2), 2);
    auto top = root ? juce::Colour(0xffffdb59) : down ? orange : juce::Colour(over ? 0xff55595a : 0xff373b3c);
    auto bottom = root   ? juce::Colour(0xffd5a21c)
                  : down ? juce::Colour(0xffb05a00)
                         : juce::Colour(0xff090b0c);
    g.setGradientFill(juce::ColourGradient(top, r.getX(), r.getY(), bottom, r.getX(), r.getBottom(), false));
    g.fillRect(r.reduced(1));
    if (root)
    {
        g.setColour(juce::Colour(0xffffe58c));
        g.drawRect(r.reduced(1.5f), 1);
    }
}
RaControl &RaSamplerEditor::control(const juce::String &id, juce::Rectangle<int> r, bool compact, bool always)
{
    auto c = std::make_unique<RaControl>(processor, id, compact);
    auto &ref = *c;
    canvas.addAndMakeVisible(ref);
    ref.setBounds(r);
    (always ? persistent : controls).push_back(std::move(c));
    return ref;
}
RaSamplerEditor::RaSamplerEditor(RaSamplerProcessor &p)
    : AudioProcessorEditor(p), processor(p), wave(p), filterGraph(p), velocity(p), keyboard(p.keyboard, p),
      tooltip(this, 650)
{
    setLookAndFeel(&look);
    addAndMakeVisible(canvas);
    canvas.setInterceptsMouseClicks(false, true);
    canvas.addMouseListener(this, true);
    juce::StringArray names{"Effects", "Sample", "Pitch/Osc", "Filter/Global", "Modulation", "MIDI"};
    for (int i = 0; i < 6; ++i)
    {
        canvas.addAndMakeVisible(tabs[i]);
        tabs[i].setButtonText(names[i]);
        tabs[i].setBounds(16 + i * 255, 14, 244, 52);
        tabs[i].onClick = [this, i] { selectTab(i); };
        tabs[i].setClickingTogglesState(false);
    }
    for (auto *c : std::initializer_list<juce::Component *>{
             &wave, &filterGraph, &velocity, &keyboard, &filename, &envelopeToggle, &previous, &next,
             &octDown, &octUp, &savePreset, &loadPreset, &globalReset, &panic, &status, &learn, &clearLearn,
             &learnChoice, &sizeChoice})
        canvas.addAndMakeVisible(c);
    wave.setBounds(276, 128, 792, 98);
    filterGraph.setBounds(1108, 132, 180, 122);
    velocity.setBounds(1144, 321, 120, 50);
    filename.setBounds(306, 94, 528, 27);
    envelopeToggle.setBounds(872, 94, 80, 27);
    envelopeToggle.setTooltip("Show or hide the editable amplitude ADSR over the sample waveform");
    envelopeToggle.onClick = [this]
    {
        processor.waveformEnvelope = !processor.waveformEnvelope.load();
        envelopeToggle.setToggleState(processor.waveformEnvelope.load(), juce::dontSendNotification);
        wave.repaint();
    };
    envelopeToggle.setToggleState(processor.waveformEnvelope.load(), juce::dontSendNotification);
    previous.setBounds(276, 94, 25, 27);
    next.setBounds(839, 94, 25, 27);
    filename.onClick = [this] { openSample(); };
    previous.onClick = [this] { adjacent(-1); };
    next.onClick = [this] { adjacent(1); };
    filename.setTooltip("Open WAV, AIFF, FLAC, OGG or MP3");
    int x = 448;
    for (auto id : {"fadeIn", "fadeOut", "sampleGain"})
    {
        control(id, {x, 231, 144, 35}, true, true);
        x += 152;
    }
    control("filterType", {1298, 91, 154, 48}, true, true);
    control("filterOn", {1108, 92, 174, 28}, true, true);
    control("cutoff", {1300, 144, 76, 112}, false, true);
    control("resonance", {1380, 144, 76, 112}, false, true);
    control("drive", {1460, 144, 76, 112}, false, true);
    control("volume", {1276, 297, 83, 83}, false, true);
    control("pan", {1363, 297, 83, 83}, false, true);
    control("transpose", {1450, 297, 83, 83}, false, true);
    keyboard.setBounds(112, 296, 993, 79);
    keyboard.setKeyWidth(23.6f);
    keyboard.setAvailableRange(0, 127);
    keyboard.setLowestVisibleKey(24);
    keyboard.setScrollButtonsVisible(true);
    keyboard.setScrollButtonWidth(1);
    keyboard.setOctaveForMiddleC(3);
    keyboard.setWantsKeyboardFocus(true);
    octDown.setBounds(28, 339, 65, 27);
    octUp.setBounds(28, 307, 65, 27);
    octDown.onClick = [this]
    {
        octave = juce::jmax(-2, octave - 1);
        keyboard.setLowestVisibleKey(24 + octave * 12);
    };
    octUp.onClick = [this]
    {
        octave = juce::jmin(3, octave + 1);
        keyboard.setLowestVisibleKey(24 + octave * 12);
    };
    sizeChoice.setName("Interface Size");
    sizeChoice.setBounds(980, 711, 128, 23);
    sizeChoice.setTooltip(
        "Interface size. You can also drag the lower-right corner to resize proportionally.");
    for (int percent : {60, 75, 90, 100, 125, 150})
        sizeChoice.addItem(juce::String(percent) + "%", percent);
    sizeChoice.onChange = [this]
    {
        int percent = sizeChoice.getSelectedId();
        if (percent > 0)
            setSize(juce::roundToInt(1560. * percent / 100.), juce::roundToInt(740. * percent / 100.));
    };
    savePreset.setBounds(1120, 711, 64, 23);
    loadPreset.setBounds(1192, 711, 64, 23);
    globalReset.setBounds(1264, 711, 126, 23);
    panic.setBounds(1398, 711, 68, 23);
    globalReset.setTooltip("Restore all sound parameters, effect order and MIDI mappings to defaults. Keep "
                           "the loaded sample and window size.");
    savePreset.onClick = [this] { preset(true); };
    loadPreset.onClick = [this] { preset(false); };
    globalReset.onClick = [this]
    {
        processor.globalReset();
        octave = 0;
        selectedFx = 0;
        keyboard.setLowestVisibleKey(24);
        envelopeToggle.setToggleState(true, juce::dontSendNotification);
        buildTab();
    };
    panic.onClick = [this]
    {
        processor.panic = true;
        processor.keyboard.reset();
    };
    status.setBounds(156, 709, 760, 25);
    status.setFont(juce::FontOptions(12));
    status.setColour(juce::Label::textColourId, muted);
    for (int i = 0; i < 5; ++i)
    {
        auto &b = fxButtons[i];
        canvas.addAndMakeVisible(b);
        b.setBounds(26, 123 + i * 27, 194, 25);
        b.onClick = [this, i]
        {
            int fx = int(processor.value("fxOrder" + juce::String(i)));
            processor.setParameter(effectIDs[fx], fxButtons[i].getToggleState() ? 1.f : 0.f);
            selectedFx = i;
        };
    }
    for (size_t i = 0; i < ra::specs().size(); ++i)
        learnChoice.addItem(ra::specs()[i].id, int(i) + 1);
    learnChoice.setSelectedId(1);
    learn.onClick = [this]
    {
        processor.learnTarget = learnChoice.getSelectedId() - 1;
        processor.setStatus("MIDI Learn armed | move one CC controller");
    };
    clearLearn.onClick = [this]
    {
        processor.clearMappings();
        processor.setStatus("MIDI mappings cleared");
    };
    setResizable(true, true);
    setResizeLimits(936, 444, 2340, 1110);
    if (auto *bounds = getConstrainer())
        bounds->setFixedAspectRatio(1560. / 740.);
    setSize(1170, 555);
    selectTab(processor.selectedTab.load());
    rootNoteChanged();
    startTimerHz(25);
}
RaSamplerEditor::~RaSamplerEditor()
{
    stopTimer();
    canvas.removeMouseListener(this);
    setLookAndFeel(nullptr);
}
void RaSamplerEditor::selectTab(int n)
{
    tab = juce::jlimit(0, 5, n);
    processor.selectedTab = tab;
    for (int i = 0; i < 6; ++i)
        tabs[i].setToggleState(i == tab, juce::dontSendNotification);
    buildTab();
    repaint();
}
void RaSamplerEditor::card(int x, int y, int w, int h, const juce::String &title,
                           const juce::String &subtitle, std::initializer_list<const char *> ids)
{
    cards.push_back({{x, y, w, h}, title, subtitle});
    int columns = w < 280 ? 2 : (w < 430 ? 3 : 4);
    int rows = (int(ids.size()) + columns - 1) / columns;
    int cellH = juce::jmin(100, (h - 62) / juce::jmax(1, rows)), cellW = (w - 20) / columns;
    int index = 0;
    for (auto id : ids)
    {
        int col = index % columns, row = index / columns;
        control(id, {x + 10 + col * cellW, y + 56 + row * cellH, cellW, cellH}, cellH < 60);
        ++index;
    }
}
void RaSamplerEditor::buildTab()
{
    loopModes.reset();
    controls.clear();
    cards.clear();
    showLearn = tab == 5;
    learn.setVisible(showLearn);
    clearLearn.setVisible(showLearn);
    learnChoice.setVisible(showLearn);
    switch (tab)
    {
    case 0:
        card(14, 400, 300, 298, "DELAY", "Rhythmic echoes",
             {"delayOn", "delaySync", "delayTime", "delayDivision", "delayFeedback", "delayHP", "delayLP",
              "delayMix"});
        card(322, 400, 300, 298, "REVERB", "Space & ambience",
             {"reverbOn", "reverbSize", "reverbDecay", "reverbPre", "reverbDamping", "reverbMix"});
        card(630, 400, 300, 298, "CHORUS", "Width & movement",
             {"chorusOn", "chorusVoices", "chorusRate", "chorusDepth", "chorusMix"});
        card(938, 400, 300, 298, "PHASER", "Phase modulation",
             {"phaserOn", "phaserStages", "phaserRate", "phaserDepth", "phaserFeedback", "phaserMix"});
        card(1246, 400, 300, 298, "SATURATION", "Harmonics & character",
             {"saturationOn", "saturationType", "saturationDrive", "saturationOutput", "saturationMix"});
        break;
    case 1:
        card(14, 400, 504, 298, "SAMPLE", "Playback boundaries | positions in samples",
             {"start", "end", "root", "sampleGain", "fadeIn", "fadeOut", "snap", "reverse"});
        card(526, 400, 504, 298, "LOOP", "Loop region | sustain and release playback",
             {"loop", "loopStart", "loopEnd", "playMode"});
        loopModes = std::make_unique<RaLoopModes>(processor);
        canvas.addAndMakeVisible(*loopModes);
        loopModes->setBounds(540, 563, 476, 121);
        card(1038, 400, 508, 298, "TIME-STRETCH", "Offline WSOLA | change duration, retain pitch",
             {"stretch"});
        break;
    case 2:
        card(14, 400, 380, 298, "PITCH", "Tune the sample", {"root", "transpose", "detune", "stretch"});
        card(402, 400, 566, 298, "PITCH ENVELOPE", "Shape pitch over time",
             {"pitchAttack", "pitchDecay", "pitchSustain", "pitchRelease", "pitchAmount"});
        card(976, 400, 570, 298, "OSCILLATOR", "Additional sound source | shares amplitude envelope",
             {"oscOn", "oscWave", "oscLevel", "oscTune"});
        break;
    case 3:
        card(14, 400, 450, 298, "FILTER", "Per-voice filter | sample and oscillator",
             {"filterOn", "filterType", "cutoff", "resonance", "drive", "peqGain"});
        card(472, 400, 464, 298, "AMPLITUDE", "Per-voice ADSR",
             {"ampAttack", "ampDecay", "ampSustain", "ampRelease"});
        card(944, 400, 602, 298, "GLOBAL", "Instrument settings",
             {"volume", "pan", "polyphony", "glide", "playMode", "loop"});
        break;
    case 4:
        card(14, 400, 262, 298, "LFO 1", "Modulation source",
             {"lfo1Wave", "lfo1Sync", "lfo1Rate", "lfo1Division", "lfo1Retrigger"});
        card(284, 400, 262, 298, "LFO 2", "Modulation source",
             {"lfo2Wave", "lfo2Sync", "lfo2Rate", "lfo2Division", "lfo2Retrigger"});
        card(554, 400, 310, 298, "FILTER ENVELOPE", "Cutoff envelope",
             {"filterAttack", "filterDecay", "filterSustain", "filterRelease", "filterAmount"});
        cards.push_back(
            {{872, 400, 674, 298}, "MODULATION MATRIX", "Source -> destination | bipolar amount"});
        for (int i = 0; i < 8; ++i)
        {
            int y = 452 + i * 29;
            control("mod" + juce::String(i) + "Source", {886, y, 222, 28}, true).label.setVisible(false);
            control("mod" + juce::String(i) + "Dest", {1110, y, 221, 28}, true).label.setVisible(false);
            control("mod" + juce::String(i) + "Amount", {1338, y, 193, 28}, true).label.setVisible(false);
        }
        break;
    case 5:
        card(14, 400, 450, 298, "MIDI RESPONSE", "One sample across the keyboard",
             {"midiChannel", "root", "velocityCurve", "bendRange", "aftertouchEnable", "sustainEnable"});
        cards.push_back({{472, 400, 602, 298},
                         "MIDI LEARN",
                         "Select a parameter, press Learn CC, then move a controller"});
        learnChoice.setBounds(491, 477, 560, 32);
        learn.setBounds(491, 528, 170, 34);
        clearLearn.setBounds(674, 528, 170, 34);
        card(1082, 400, 464, 298, "PLAYBACK", "Note and voice behaviour",
             {"playMode", "polyphony", "glide", "ampRelease"});
        break;
    }
    for (auto &c : controls)
        c->resized();
    // Attachments inherit normalized ranges; positions are shown as samples rather than fractions.
    for (auto &c : controls)
        if (c->id == "start" || c->id == "end" || c->id == "loopStart" || c->id == "loopEnd")
        {
            c->slider.textFromValueFunction = [this](double v)
            { return juce::String(juce::int64(v * (processor.sample()->length() - 1))); };
            c->slider.valueFromTextFunction = [this](const juce::String &s)
            { return s.getDoubleValue() / juce::jmax(1, processor.sample()->length() - 1); };
            c->slider.updateText();
        }
    repaint();
}
void RaSamplerEditor::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colour(0xff080a0b));
    g.addTransform(juce::AffineTransform::scale(float(getWidth()) / 1560.f, float(getHeight()) / 740.f));
    panel(g, {3, 3, 1554, 733}, 16);
    panel(g, {8, 7, 1544, 66}, 10);
    panel(g, {14, 84, 238, 188});
    panel(g, {264, 84, 816, 188});
    panel(g, {1090, 84, 456, 188});
    panel(g, {14, 284, 1104, 104});
    panel(g, {1128, 284, 418, 104});
    write(g, "EFFECTS CHAIN", {29, 94, 192, 24}, 16);
    write(g, ":", {225, 122, 15, 140}, 17, muted, juce::Justification::centred);
    for (int i = 0; i < 5; ++i)
    {
        auto r = juce::Rectangle<float>(25, 123.f + i * 27, 216, 25);
        g.setColour(i == selectedFx ? orange.withAlpha(.16f) : black.withAlpha(.65f));
        g.fillRoundedRectangle(r, 3);
        g.setColour(muted.withAlpha(.7f));
        for (int row = 0; row < 3; ++row)
        {
            g.fillEllipse(227.f, 130.f + i * 27 + row * 4, 2, 2);
            g.fillEllipse(232.f, 130.f + i * 27 + row * 4, 2, 2);
        }
    }
    write(g, "Velocity", {1144, 293, 120, 22}, 14);
    write(g, "Octave", {27, 286, 70, 20}, 13);
    write(g, "RaSampler", {24, 710, 122, 25}, 16, orange);
    write(g, "Size", {932, 710, 43, 25}, 12, muted);
    write(g, "0.5.0", {1483, 710, 50, 24}, 12, muted, juce::Justification::centredRight);
    auto s = processor.sample();
    write(g, juce::String(s->rate / 1000., 1) + " kHz | " + juce::String(s->length() / s->rate, 2) + " s",
          {963, 94, 104, 28}, 11, muted, juce::Justification::centredRight);
    for (auto &c : cards)
    {
        panel(g, c.bounds.toFloat());
        write(g, c.title, c.bounds.reduced(16).withHeight(25), 20, orange);
        write(g, c.subtitle, {c.bounds.getX() + 16, c.bounds.getY() + 31, c.bounds.getWidth() - 32, 18}, 11,
              muted);
        g.setColour(juce::Colour(0xff555959));
        g.drawHorizontalLine(c.bounds.getY() + 53, float(c.bounds.getX() + 14),
                             float(c.bounds.getRight() - 14));
    }
    if (tab == 5)
    {
        write(g,
              processor.learnTarget.load() >= 0 ? "Waiting for MIDI CC..."
              : processor.lastLearned.load() >= 0
                  ? "Mapped controller: CC " + juce::String(processor.lastLearned.load())
                  : "Mappings are saved with the DAW project and presets.",
              {491, 582, 560, 30}, 14, processor.learnTarget.load() >= 0 ? orange : muted);
    }
    if (tab == 1)
    {
        write(g, "Open an audio file using its name above, or drop it here.", {1060, 595, 464, 24}, 13,
              muted);
        write(g, "WAV | AIFF | FLAC | OGG | MP3", {1060, 622, 464, 24}, 13, muted);
    }
    float db = juce::Decibels::gainToDecibels(level, -60.f);
    g.setColour(black);
    g.fillRect(1480, 714, 3, 17);
    g.setColour(level > 1 ? juce::Colours::red : orange);
    g.fillRect(1480.f, 731.f - 17 * juce::jlimit(0.f, 1.f, (db + 60) / 60), 3.f,
               17 * juce::jlimit(0.f, 1.f, (db + 60) / 60));
}
void RaSamplerEditor::resized()
{
    sizeChoice.setText(juce::String(juce::roundToInt(100. * getWidth() / 1560.)) + "%",
                       juce::dontSendNotification);
    canvas.setBounds(0, 0, 1560, 740);
    canvas.setTransform(juce::AffineTransform::scale(float(getWidth()) / 1560.f, float(getHeight()) / 740.f));
}
void RaSamplerEditor::rootNoteChanged(bool forceFocus)
{
    int note = juce::jlimit(ra::rootMinNote, ra::rootMaxNote, juce::roundToInt(processor.value("root")));
    if (displayedRoot == note && !forceFocus)
        return;
    displayedRoot = note;
    auto key = keyboard.getRectangleForKey(note);
    if (key.getX() < 0 || key.getRight() > keyboard.getWidth())
    {
        keyboard.setLowestVisibleKey(ra::rootMinNote);
        octave = 1;
    }
    keyboard.repaint();
}
void RaSamplerEditor::timerCallback()
{
    if (loopModes)
        loopModes->refresh();
    rootNoteChanged();
    if (tab != processor.selectedTab.load())
        selectTab(processor.selectedTab.load());
    auto s = processor.sample();
    filename.setButtonText(s->name);
    envelopeToggle.setToggleState(processor.waveformEnvelope.load(), juce::dontSendNotification);
    if (displayedLength != s->length())
    {
        displayedLength = s->length();
        for (auto *list : {&persistent, &controls})
            for (auto &c : *list)
                if (c->id == "start" || c->id == "end" || c->id == "loopStart" || c->id == "loopEnd")
                    c->slider.updateText();
    }
    for (int i = 0; i < 5; ++i)
    {
        int fx = juce::jlimit(0, 4, int(processor.value("fxOrder" + juce::String(i))));
        fxButtons[i].setButtonText(effectNames[fx]);
        fxButtons[i].setToggleState(processor.value(effectIDs[fx]) > .5f, juce::dontSendNotification);
    }
    level = juce::jmax(processor.meter.load(), level * .83f);
    status.setText((processor.stretchBusy.load() ? "Processing time-stretch..." : processor.status()) +
                       "  |  Voices " + juce::String(processor.activeVoices.load()) + "  |  " +
                       juce::String(juce::Decibels::gainToDecibels(level, -60.f), 1) + " dBFS",
                   juce::dontSendNotification);
    wave.repaint();
    filterGraph.repaint();
    velocity.repaint();
    keyboard.repaint();
    repaint();
}
bool RaSamplerEditor::isInterestedInFileDrag(const juce::StringArray &files)
{
    return files.size() == 1 && juce::File(files[0]).hasFileExtension("wav;aif;aiff;flac;ogg;mp3;rasampler");
}
void RaSamplerEditor::filesDropped(const juce::StringArray &files, int, int)
{
    if (files.size())
        loadFile(juce::File(files[0]));
}
void RaSamplerEditor::loadFile(const juce::File &f)
{
    if (f.hasFileExtension("rasampler"))
    {
        juce::MemoryBlock block;
        if (f.loadFileAsData(block))
            processor.setStateInformation(block.getData(), int(block.getSize()));
        return;
    }
    juce::String error;
    if (processor.loadSample(f, error))
    {
        lastFile = f;
        wave.repaint();
    }
    else
        processor.setStatus(error);
}
void RaSamplerEditor::openSample()
{
    chooser =
        std::make_unique<juce::FileChooser>("Load sample", lastFile, "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    chooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe = juce::Component::SafePointer<RaSamplerEditor>(this)](const juce::FileChooser &f)
        {
            if (safe && f.getResult().existsAsFile())
                safe->loadFile(f.getResult());
        });
}
void RaSamplerEditor::adjacent(int direction)
{
    if (!lastFile.existsAsFile())
    {
        openSample();
        return;
    }
    auto files = lastFile.getParentDirectory().findChildFiles(juce::File::findFiles, false, "*");
    files.removeIf([](const juce::File &f) { return !f.hasFileExtension("wav;aif;aiff;flac;ogg;mp3"); });
    files.sort();
    int index = files.indexOf(lastFile);
    if (files.size())
        loadFile(files[(index + direction + files.size()) % files.size()]);
}
void RaSamplerEditor::preset(bool save)
{
    chooser = std::make_unique<juce::FileChooser>(save ? "Save RaSampler preset" : "Load RaSampler preset",
                                                  juce::File(), "*.rasampler");
    int flags =
        juce::FileBrowserComponent::canSelectFiles |
        (save ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting
              : juce::FileBrowserComponent::openMode);
    chooser->launchAsync(
        flags,
        [safe = juce::Component::SafePointer<RaSamplerEditor>(this), save](const juce::FileChooser &f)
        {
            if (!safe || f.getResult() == juce::File())
                return;
            if (save)
            {
                juce::MemoryBlock data;
                safe->processor.getStateInformation(data);
                auto file = f.getResult().withFileExtension("rasampler");
                safe->processor.setStatus(file.replaceWithData(data.getData(), data.getSize())
                                              ? "Preset saved | sample embedded"
                                              : "Could not save preset");
            }
            else
                safe->loadFile(f.getResult());
        });
}
void RaSamplerEditor::mouseDown(const juce::MouseEvent &e)
{
    auto pos = e.getEventRelativeTo(&canvas).position;
    if (pos.x > 220 && pos.x < 245 && pos.y >= 123 && pos.y < 258)
    {
        dragFrom = int((pos.y - 123) / 27);
        selectedFx = dragFrom;
    }
}
void RaSamplerEditor::mouseDrag(const juce::MouseEvent &e)
{
    if (dragFrom >= 0)
    {
        auto pos = e.getEventRelativeTo(&canvas).position;
        processor.setStatus("Move " + fxButtons[dragFrom].getButtonText() + " to chain slot " +
                            juce::String(juce::jlimit(1, 5, int((pos.y - 123) / 27) + 1)));
    }
}
void RaSamplerEditor::mouseUp(const juce::MouseEvent &e)
{
    if (dragFrom >= 0)
    {
        auto pos = e.getEventRelativeTo(&canvas).position;
        int to = juce::jlimit(0, 4, int((pos.y - 123) / 27));
        processor.reorder(dragFrom, to);
        selectedFx = to;
        dragFrom = -1;
    }
}
