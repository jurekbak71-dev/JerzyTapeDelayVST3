#pragma once
#include <JuceHeader.h>
#include "FXDSP.h"

class JerzyFXBlockAudioProcessor : public juce::AudioProcessor
{
public:
    JerzyFXBlockAudioProcessor();
    ~JerzyFXBlockAudioProcessor() override=default;
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    const juce::String getName() const override {return "Jerzy FX Block";}
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    bool isMidiEffect() const override {return false;}
    double getTailLengthSeconds() const override {return 8.0;}
    int getNumPrograms() override {return 1;} int getCurrentProgram() override{return 0;}
    void setCurrentProgram(int) override{} const juce::String getProgramName(int) override{return {};}
    void changeProgramName(int,const juce::String&) override{}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setEffectOrder(const std::array<int,6>&);
    std::array<int,6> getEffectOrder() const;
private:
    jerzyfx::FXEngine engine;
    std::array<std::atomic<int>,6> fxOrder {{{0},{1},{2},{3},{4},{5}}};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JerzyFXBlockAudioProcessor)
};
