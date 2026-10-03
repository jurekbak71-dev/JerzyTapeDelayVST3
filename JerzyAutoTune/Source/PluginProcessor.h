#pragma once

#include <JuceHeader.h>
#include <signalsmith-stretch/signalsmith-stretch.h>
#include <array>
#include <atomic>

class JerzyAutoTuneAudioProcessorEditor;

class JerzyAutoTuneAudioProcessor final : public juce::AudioProcessor
{
public:
    JerzyAutoTuneAudioProcessor();
    ~JerzyAutoTuneAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "JERZY AUTO TUNE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.08; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    float getOutputPeak() const noexcept { return outputPeak.load(std::memory_order_relaxed); }
    float getInputPeak() const noexcept { return inputPeak.load(std::memory_order_relaxed); }

private:
    friend class JerzyAutoTuneAudioProcessorEditor;
    static APVTS::ParameterLayout createParameterLayout();
    void analysePitch() noexcept;
    float tunedRatio() const noexcept;
    void processVocalChain(juce::AudioBuffer<float>& buffer) noexcept;

    struct Biquad
    {
        std::array<float, 5> coefficients { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        std::array<float, 5> targetCoefficients { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
        void setTarget(const std::array<float, 5>& c) noexcept { targetCoefficients = c; }
        void reset() noexcept { x1 = x2 = y1 = y2 = 0.0f; }
        float process(float input, float coefficientSmoothing) noexcept;
    };

    static constexpr int detectorSize = 4096;
    std::array<float, detectorSize> detector {};
    int detectorWrite = 0;
    int samplesSinceAnalysis = 0;
    float detectedMidi = -1.0f;
    float targetPitchRatio = 1.0f;
    float smoothedRatio = 1.0f;
    double currentSampleRate = 44100.0;

    int maximumBlockSize = 512;
    int dryDelayLength = 1;
    int dryDelayWrite = 0;
    int seekInputLength = 0;
    int startupInputCount = 0;
    bool stretcherReady = false;
    juce::AudioBuffer<float> stretchedBuffer;
    juce::AudioBuffer<float> dryDelayBuffer;
    juce::AudioBuffer<float> startupBuffer;
    signalsmith::stretch::SignalsmithStretch<float> stretcher;
    juce::SmoothedValue<float> mixSmoother;
    juce::SmoothedValue<float> speedSmoother;

    std::array<float, 2> gateEnvelope {};
    std::array<float, 2> noiseHpState {};
    std::array<float, 2> noisePrevInput {};
    std::array<float, 2> deEssSideState {};
    std::array<float, 2> deEssPrevInput {};
    float deEssEnvelope = 0.0f;
    float compressorEnvelope = 0.0f;
    float doublerPhase = 0.0f;
    int doublerWrite = 0;
    int doublerBufferLength = 1;
    juce::AudioBuffer<float> doublerBuffer;
    std::array<std::array<Biquad, 3>, 2> vocalEq;

    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JerzyAutoTuneAudioProcessor)
};
