#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class JerzyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    JerzyLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int w, int h, bool, int, int, int, int,
                      juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
};

class JerzyMonoAnalogAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit JerzyMonoAnalogAudioProcessorEditor(JerzyMonoAnalogAudioProcessor&);
    ~JerzyMonoAnalogAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Section
    {
        juce::String title;
        juce::Colour led;
        juce::Rectangle<float> norm;
    };

    class OutputMeter : public juce::Component
    {
    public:
        void setLevel(float newLevel) { level = juce::jlimit(0.0f, 1.0f, newLevel); repaint(); }
        void paint(juce::Graphics&) override;
    private:
        float level = 0.0f;
    };

    void timerCallback() override;
    void setupKnob(juce::Slider&, const juce::String& suffix = {});
    void setupCombo(juce::ComboBox&, const juce::StringArray&);
    void setupToggle(juce::ToggleButton&, const juce::String&, juce::Colour);
    void place(juce::Component&, float x, float y, float w, float h);
    void addSection(const juce::String&, juce::Colour, float x, float y, float w, float h);
    void drawSection(juce::Graphics&, const Section&) const;
    void drawEnvelope(juce::Graphics&, juce::Rectangle<float>, bool filter) const;
    float sx() const noexcept { return getWidth() / 1440.0f; }
    float sy() const noexcept { return getHeight() / 720.0f; }
    float s()  const noexcept { return juce::jmin(sx(), sy()); }

    JerzyMonoAnalogAudioProcessor& proc;
    JerzyLookAndFeel look;
    std::vector<Section> sections;

    juce::Label title, subtitle, preset, scaleLabel;

    juce::ComboBox osc1Wave, osc1Oct, osc2Wave, osc2Oct, subWave, lfoWave, glideMode, priority;
    juce::Slider osc1Level, pulseWidth, osc2Level, detune, subLevel, noiseLevel, mixDrive, drift;
    juce::Slider cutoff, resonance, filterDrive, filterEnv, keyTrack;
    juce::Slider aA, aD, aS, aR, fA, fD, fS, fR;
    juce::Slider lfoRate, lfoPitch, lfoFilter, lfoPWM;
    juce::Slider glide, outDrive, master;
    juce::ToggleButton legato, retrigger;
    OutputMeter outputMeter;

    std::unique_ptr<ComboAttachment> osc1WaveA, osc1OctA, osc2WaveA, osc2OctA, subWaveA, lfoWaveA, glideModeA, priorityA;
    std::unique_ptr<SliderAttachment> osc1LevelA, pulseWidthA, osc2LevelA, detuneA, subLevelA, noiseLevelA, mixDriveA, driftA;
    std::unique_ptr<SliderAttachment> cutoffA, resonanceA, filterDriveA, filterEnvA, keyTrackA;
    std::unique_ptr<SliderAttachment> aAA, aDA, aSA, aRA, fAA, fDA, fSA, fRA;
    std::unique_ptr<SliderAttachment> lfoRateA, lfoPitchA, lfoFilterA, lfoPWMA;
    std::unique_ptr<SliderAttachment> glideA, outDriveA, masterA;
    std::unique_ptr<ButtonAttachment> legatoA, retriggerA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JerzyMonoAnalogAudioProcessorEditor)
};
