#pragma once

#include "PluginProcessor.h"

class JerzyAutoTuneAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit JerzyAutoTuneAudioProcessorEditor(JerzyAutoTuneAudioProcessor&);
    ~JerzyAutoTuneAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    class AnalogLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int, int, int, int, float,
                              float, float, juce::Slider&) override;
    };

    struct ModuleControls
    {
        juce::ToggleButton enabled { "ON" };
        std::array<juce::Slider*, 4> sliders { nullptr, nullptr, nullptr, nullptr };
        std::array<juce::Label*, 4> labels { nullptr, nullptr, nullptr, nullptr };
        int count = 0;
    };

    void timerCallback() override;
    void configureSlider(juce::Slider&, const juce::String& suffix);
    void configureLabel(juce::Label&, const juce::String& text);
    juce::Rectangle<int> scaledBounds(float x, float y, float w, float h) const;
    void layoutModule(ModuleControls&, float x, float y, float w, float h);
    void drawModule(juce::Graphics&, const juce::String& title, float x, float y, float w, float h, bool enabled);

    JerzyAutoTuneAudioProcessor& processor;
    AnalogLookAndFeel analogLookAndFeel;

    juce::ComboBox keyBox, scaleBox;
    std::array<juce::TextButton, 12> noteButtons;
    juce::Slider speedSlider, amountSlider, mixSlider;
    juce::Label keyLabel, scaleLabel, speedLabel, amountLabel, mixLabel;

    juce::Slider gateThreshold, gateRelease;
    juce::Slider noiseThreshold, noiseReduction;
    juce::Slider deEssFreq, deEssAmount;
    juce::Slider satDrive, satMix;
    juce::Slider doublerAmount, doublerDelay;
    juce::Slider compThreshold, compRatio, compMakeup, limiterCeiling;
    juce::Slider eqLow, eqMid, eqHigh, outputGain;

    juce::Label gateThresholdLabel, gateReleaseLabel;
    juce::Label noiseThresholdLabel, noiseReductionLabel;
    juce::Label deEssFreqLabel, deEssAmountLabel;
    juce::Label satDriveLabel, satMixLabel;
    juce::Label doublerAmountLabel, doublerDelayLabel;
    juce::Label compThresholdLabel, compRatioLabel, compMakeupLabel, limiterCeilingLabel;
    juce::Label eqLowLabel, eqMidLabel, eqHighLabel, outputGainLabel;

    ModuleControls gateModule, noiseModule, deEssModule, satModule, doublerModule, compModule, eqModule;

    std::unique_ptr<ComboAttachment> keyAttachment, scaleAttachment;
    std::array<std::unique_ptr<ButtonAttachment>, 12> noteAttachments;
    std::unique_ptr<SliderAttachment> speedAttachment, amountAttachment, mixAttachment;
    std::array<std::unique_ptr<SliderAttachment>, 18> vocalSliderAttachments;
    std::array<std::unique_ptr<ButtonAttachment>, 7> vocalButtonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JerzyAutoTuneAudioProcessorEditor)
};
