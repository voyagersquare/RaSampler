#pragma once
#include "PluginProcessor.h"
#include <juce_audio_utils/juce_audio_utils.h>
class RaLook final : public juce::LookAndFeel_V4
{
  public:
    RaLook();
    void drawButtonText(juce::Graphics &, juce::TextButton &, bool, bool) override;
    void drawButtonBackground(juce::Graphics &, juce::Button &, const juce::Colour &, bool, bool) override;
    void drawRotarySlider(juce::Graphics &, int, int, int, int, float, float, float, juce::Slider &) override;
    void drawToggleButton(juce::Graphics &, juce::ToggleButton &, bool, bool) override;
    void drawComboBox(juce::Graphics &, int, int, bool, int, int, int, int, juce::ComboBox &) override;
};
class RaControl : public juce::Component
{
  public:
    RaControl(RaSamplerProcessor &, const juce::String &, bool compact = false);
    void resized() override;
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &) override;
    juce::Slider slider;
    juce::ComboBox combo;
    juce::ToggleButton toggle;
    juce::Label label;
    juce::String id;
    bool compact = false;

  private:
    RaSamplerProcessor &processor;
    std::unique_ptr<ra::APVTS::SliderAttachment> sa;
    std::unique_ptr<ra::APVTS::ComboBoxAttachment> ca;
    std::unique_ptr<ra::APVTS::ButtonAttachment> ba;
};
class RaWave : public juce::Component, public juce::SettableTooltipClient
{
  public:
    explicit RaWave(RaSamplerProcessor &p) : processor(p) {}
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void mouseMove(const juce::MouseEvent &) override;
    std::array<juce::Point<float>, 4> envelopeHandles() const;
    std::array<juce::Rectangle<float>, 2> loopHandleBounds() const;

  private:
    struct EnvelopeGeometry
    {
        juce::Rectangle<float> bounds;
        std::array<juce::Point<float>, 4> handles;
        float previewMs = 500;
    };
    EnvelopeGeometry envelopeGeometry() const;
    void paintEnvelope(juce::Graphics &);
    void paintLoopMarkers(juce::Graphics &);
    int loopHit(juce::Point<float>) const;
    int envelopeHit(juce::Point<float>) const;
    std::vector<juce::String> envelopeParameters;
    juce::Point<float> envelopeMouseDown;
    float dragPreviewMs = 500, dragWidth = 1, dragHeight = 1;
    float initialAttack = 1, initialDecay = 200, initialSustain = 1, initialRelease = 100;
    int envelopeDrag = -1;
    RaSamplerProcessor &processor;
    juce::String dragging;
    float loopDragOffset = 0;
    int cachedLength = 0;
    std::shared_ptr<const ra::SampleData> cached;
    juce::Path waveform;
};
class RaFilterGraph : public juce::Component
{
  public:
    explicit RaFilterGraph(RaSamplerProcessor &p) : processor(p) {}
    void paint(juce::Graphics &) override;
    void mouseDrag(const juce::MouseEvent &) override;

  private:
    RaSamplerProcessor &processor;
};
class RaVelocity : public juce::Component
{
  public:
    explicit RaVelocity(RaSamplerProcessor &p) : processor(p) {}
    void paint(juce::Graphics &) override;
    void mouseDown(const juce::MouseEvent &e) override
    {
        mouseDrag(e);
    }
    void mouseDrag(const juce::MouseEvent &) override;

  private:
    RaSamplerProcessor &processor;
};
class RaKeyboard : public juce::MidiKeyboardComponent
{
  public:
    RaKeyboard(juce::MidiKeyboardState &k, RaSamplerProcessor &p)
        : MidiKeyboardComponent(k, horizontalKeyboard), processor(p)
    {
    }
    void drawWhiteNote(int, juce::Graphics &, juce::Rectangle<float>, bool, bool, juce::Colour,
                       juce::Colour) override;
    void drawBlackNote(int, juce::Graphics &, juce::Rectangle<float>, bool, bool, juce::Colour) override;

  private:
    RaSamplerProcessor &processor;
};
class RaSamplerEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer,
                              public juce::FileDragAndDropTarget
{
  public:
    explicit RaSamplerEditor(RaSamplerProcessor &);
    ~RaSamplerEditor() override;
    void paint(juce::Graphics &) override;
    void resized() override;
    bool isInterestedInFileDrag(const juce::StringArray &) override;
    void filesDropped(const juce::StringArray &, int, int) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;
    void selectTab(int);
    void openSample();
    void loadFile(const juce::File &);
    void rootNoteChanged(bool forceFocus = false);

  private:
    RaSamplerProcessor &processor;
    RaLook look;
    juce::Component canvas;
    RaWave wave;
    RaFilterGraph filterGraph;
    RaVelocity velocity;
    RaKeyboard keyboard;
    juce::TextButton tabs[6], filename, envelopeToggle{"ADSR"}, previous{"<"}, next{">"}, octDown{"-"},
        octUp{"+"}, savePreset{"Save"}, loadPreset{"Load"}, globalReset{"Global Reset"}, panic{"Panic"},
        learn{"Learn CC"}, clearLearn{"Clear map"};
    juce::ToggleButton fxButtons[5];
    juce::ComboBox learnChoice, sizeChoice;
    std::vector<std::unique_ptr<RaControl>> persistent, controls;
    struct Card
    {
        juce::Rectangle<int> bounds;
        juce::String title, subtitle;
    };
    std::vector<Card> cards;
    juce::Label status;
    juce::TooltipWindow tooltip;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File lastFile;
    int tab = 0, octave = 0, dragFrom = -1, selectedFx = 0, displayedLength = -1, displayedRoot = -1;
    float level = 0;
    bool showLearn = false;
    void timerCallback() override;
    void buildTab();
    RaControl &control(const juce::String &, juce::Rectangle<int>, bool compact = false, bool always = false);
    void card(int, int, int, int, const juce::String &, const juce::String &,
              std::initializer_list<const char *>);
    void preset(bool);
    void adjacent(int);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RaSamplerEditor)
};
