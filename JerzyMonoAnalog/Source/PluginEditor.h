#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class JerzyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    JerzyLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float,
                          juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
};

class ResetSlider : public juce::Slider
{
public:
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        {
            const double target = (getMinimum() <= 0.0 && getMaximum() >= 0.0) ? 0.0 : getMinimum();
            setValue(target, juce::sendNotificationSync);
            return;
        }
        juce::Slider::mouseDown(e);
    }
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

    struct Section { juce::String title; juce::Colour led; juce::Rectangle<float> norm; };

    class OutputMeter : public juce::Component
    {
    public:
        void setLevel(float v) { level = juce::jlimit(0.0f, 1.0f, v); repaint(); }
        void paint(juce::Graphics&) override;
    private:
        float level = 0.0f;
    };

    void timerCallback() override;
    void setupKnob(ResetSlider&, const juce::String& name, const juce::String& unit = {});
    void setupEnvSlider(ResetSlider&, const juce::String& name, const juce::String& unit = {});
    void setupCombo(juce::ComboBox&, const juce::StringArray&);
    void setupToggle(juce::ToggleButton&, const juce::String&, juce::Colour);
    void place(juce::Component&, float, float, float, float);
    void addSection(const juce::String&, juce::Colour, float, float, float, float);
    void drawSection(juce::Graphics&, const Section&) const;
    void drawEnvelope(juce::Graphics&, juce::Rectangle<float>, bool) const;
    void drawCtlLabel(juce::Graphics&, const juce::String&, float, float, float) const;
    float sx() const noexcept { return getWidth() / 1440.0f; }
    float sy() const noexcept { return getHeight() / 720.0f; }
    float s() const noexcept { return juce::jmin(sx(), sy()); }

    JerzyMonoAnalogAudioProcessor& proc;
    JerzyLookAndFeel look;
    std::vector<Section> sections;
    juce::Label title, subtitle, preset, scaleLabel;

    juce::ComboBox osc1Wave, osc1Oct, osc2Wave, osc2Oct, subWave, lfoWave, lfoDivision, glideMode, priority;
    ResetSlider osc1Level, pulseWidth, osc2Level, detune, subLevel, noiseLevel, mixDrive, drift;
    ResetSlider cutoff, resonance, filterDrive, filterEnv, keyTrack;
    ResetSlider aA, aD, aS, aR, fA, fD, fS, fR;
    ResetSlider lfoRate, lfoPitch, lfoFilter, lfoPWM, lfoAmp, lfoFade;
    ResetSlider glide, outDrive, master;
    juce::ToggleButton legato, retrigger, lfoSync;
    OutputMeter outputMeter;

    std::unique_ptr<ComboAttachment> osc1WaveA, osc1OctA, osc2WaveA, osc2OctA, subWaveA, lfoWaveA, lfoDivisionA, glideModeA, priorityA;
    std::unique_ptr<SliderAttachment> osc1LevelA, pulseWidthA, osc2LevelA, detuneA, subLevelA, noiseLevelA, mixDriveA, driftA;
    std::unique_ptr<SliderAttachment> cutoffA, resonanceA, filterDriveA, filterEnvA, keyTrackA;
    std::unique_ptr<SliderAttachment> aAA, aDA, aSA, aRA, fAA, fDA, fSA, fRA;
    std::unique_ptr<SliderAttachment> lfoRateA, lfoPitchA, lfoFilterA, lfoPWMA, lfoAmpA, lfoFadeA;
    std::unique_ptr<SliderAttachment> glideA, outDriveA, masterA;
    std::unique_ptr<ButtonAttachment> legatoA, retriggerA, lfoSyncA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JerzyMonoAnalogAudioProcessorEditor)
};
