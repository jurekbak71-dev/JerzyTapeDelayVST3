#pragma once

#include "PluginProcessor.h"

class ToneSnapAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit ToneSnapAudioProcessorEditor(ToneSnapAudioProcessor&);
    ~ToneSnapAudioProcessorEditor() override;

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

    void timerCallback() override;
    juce::Rectangle<int> scaledBounds(float x, float y, float width, float height) const;
    void configureSlider(juce::Slider&, const juce::String& suffix);

    ToneSnapAudioProcessor& processor;
    AnalogLookAndFeel analogLookAndFeel;
    juce::ComboBox keyBox, scaleBox;
    juce::Slider speedSlider, amountSlider, mixSlider;
    juce::Slider thresholdSlider, ratioSlider, makeupSlider;
    juce::Slider lowSlider, midSlider, highSlider, outputSlider;
    juce::ToggleButton compressorButton { "ON" }, equalizerButton { "ON" };
    juce::Label keyLabel, scaleLabel, speedLabel, amountLabel, mixLabel;
    juce::Label thresholdLabel, ratioLabel, makeupLabel;
    juce::Label lowLabel, midLabel, highLabel, outputLabel;
    std::unique_ptr<ComboAttachment> keyAttachment, scaleAttachment;
    std::unique_ptr<SliderAttachment> speedAttachment, amountAttachment, mixAttachment;
    std::unique_ptr<SliderAttachment> thresholdAttachment, ratioAttachment, makeupAttachment;
    std::unique_ptr<SliderAttachment> lowAttachment, midAttachment, highAttachment, outputAttachment;
    std::unique_ptr<ButtonAttachment> compressorAttachment, equalizerAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToneSnapAudioProcessorEditor)
};
