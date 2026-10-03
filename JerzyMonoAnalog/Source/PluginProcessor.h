#pragma once
#include <JuceHeader.h>
#include "AnalogDSP.h"

class JerzyMonoAnalogAudioProcessor : public juce::AudioProcessor
{
public:
    JerzyMonoAnalogAudioProcessor();
    ~JerzyMonoAnalogAudioProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Jerzy Mono Analog"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float getOutputMeter() const noexcept { return outputMeter.load(); }
private:
    int getChoiceIndex(const char* id) const;
    int chooseArpNote(int pattern, int step);
    bool arpRhythmGate(int rhythm, int step) const;
    void resetArpState();
    jerzy::MonoAnalogEngine engine;
    std::atomic<float> outputMeter { 0.0f };
    double currentSampleRate = 44100.0;
    double arpSamplesToNext = 0.0;
    int arpStep = 0;
    int arpCurrentNote = -1;
    int arpUpDownPos = 0;
    juce::Array<int> arpHeldNotes;
    juce::Array<int> arpLatchedNotes;
    juce::Array<int> physicalHeldNotes;
    std::mt19937 arpRng { 0x51a7u };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JerzyMonoAnalogAudioProcessor)
};
