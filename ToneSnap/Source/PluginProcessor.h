#pragma once

#include <JuceHeader.h>
#include <signalsmith-stretch/signalsmith-stretch.h>
#include <array>

class ToneSnapAudioProcessorEditor;

class ToneSnapAudioProcessor final : public juce::AudioProcessor
{
public:
    ToneSnapAudioProcessor();
    ~ToneSnapAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ToneSnap"; }
    bool acceptsMidi() const override { return false; }
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

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;

private:
    friend class ToneSnapAudioProcessorEditor;
    static APVTS::ParameterLayout createParameterLayout();
    void analysePitch() noexcept;
    float tunedRatio() const noexcept;

    static constexpr int detectorSize = 2048;
    std::array<float, detectorSize> detector{};
    int detectorWrite = 0;
    int samplesSinceAnalysis = 0;
    float detectedMidi = -1.0f;
    float targetPitchRatio = 1.0f;
    float smoothedRatio = 1.0f;
    double currentSampleRate = 44100.0;
    int maximumBlockSize = 512;
    int dryDelayLength = 1;
    int dryDelayWrite = 0;
    juce::AudioBuffer<float> stretchedBuffer;
    juce::AudioBuffer<float> dryDelayBuffer;
    signalsmith::stretch::SignalsmithStretch<float> stretcher;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToneSnapAudioProcessor)
};
