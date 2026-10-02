#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
class JerzyMonoAnalogAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit JerzyMonoAnalogAudioProcessorEditor(JerzyMonoAnalogAudioProcessor& p) : AudioProcessorEditor(&p), proc(p) { setResizable(true,true); setSize(900,520); }
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff202020)); g.setColour(juce::Colours::white); g.setFont(28.0f); g.drawFittedText("JERZY MONO ANALOG - DSP BUILD", getLocalBounds(), juce::Justification::centred, 1); }
    void resized() override {}
private: JerzyMonoAnalogAudioProcessor& proc;
};
