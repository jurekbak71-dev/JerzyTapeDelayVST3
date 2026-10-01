#pragma once

#include "PluginProcessor.h"

class ToneSnapAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ToneSnapAudioProcessorEditor(ToneSnapAudioProcessor&);
    ~ToneSnapAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    ToneSnapAudioProcessor& processor;
    juce::ComboBox keyBox, scaleBox;
    juce::Slider retuneSlider, amountSlider, mixSlider;
    juce::Label keyLabel, scaleLabel, retuneLabel, amountLabel, mixLabel;
    juce::Label keyValueLabel, scaleValueLabel;
    std::unique_ptr<ComboAttachment> keyAttachment, scaleAttachment;
    std::unique_ptr<SliderAttachment> retuneAttachment, amountAttachment, mixAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToneSnapAudioProcessorEditor)
};
