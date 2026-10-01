#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const juce::StringArray noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
const juce::StringArray scaleNames { "Chromatic", "Major", "Minor" };

}

ToneSnapAudioProcessor::ToneSnapAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{}

ToneSnapAudioProcessor::APVTS::ParameterLayout ToneSnapAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterChoice>("key", "Key", noteNames, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("scale", "Scale", scaleNames, 1));
    layout.add(std::make_unique<juce::AudioParameterFloat>("speed", "Retune", juce::NormalisableRange<float>(0.0f, 1.0f), 0.72f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("amount", "Amount", juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    return layout;
}

void ToneSnapAudioProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    currentSampleRate = sampleRate;
    detector.fill(0.0f);
    detectorWrite = samplesSinceAnalysis = 0;
    detectedMidi = -1.0f;
    targetPitchRatio = 1.0f;
    smoothedRatio = 1.0f;
    maximumBlockSize = juce::jmax(1, maximumExpectedSamplesPerBlock);
    const int channels = juce::jlimit(1, 2, getTotalNumOutputChannels());
    stretchedBuffer.setSize(channels, maximumBlockSize, false, true, true);
    stretcher.presetDefault(channels, static_cast<float>(sampleRate), true);
    seekInputLength = stretcher.outputSeekLength(1.0f);
    startupBuffer.setSize(channels, seekInputLength, false, true, true);
    startupBuffer.clear();
    startupInputCount = 0;
    stretcherReady = false;
    const int latency = stretcher.inputLatency() + stretcher.outputLatency();
    setLatencySamples(latency);
    dryDelayLength = latency + maximumBlockSize + 1;
    dryDelayBuffer.setSize(channels, dryDelayLength, false, true, true);
    dryDelayBuffer.clear();
    dryDelayWrite = 0;
}

void ToneSnapAudioProcessor::releaseResources() {}

bool ToneSnapAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo()) && input == output;
}

void ToneSnapAudioProcessor::analysePitch() noexcept
{
    // YIN-style normalized difference, deliberately limited to monophonic vocal range.
    constexpr int minLag = 50;
    constexpr int maxLag = 600;
    float best = 1.0f;
    int bestLag = 0;
    double energy = 0.0;
    for (int i = 0; i < detectorSize; ++i) energy += detector[static_cast<size_t>(i)] * detector[static_cast<size_t>(i)];
    if (energy / detectorSize < 0.000002) { detectedMidi = -1.0f; targetPitchRatio = 1.0f; return; }

    float previousScore = 1.0f;
    bool foundVoicedMinimum = false;
    for (int lag = minLag; lag <= maxLag; lag += 2)
    {
        double difference = 0.0, normA = 0.0, normB = 0.0;
        for (int i = 0; i < detectorSize - maxLag; i += 4)
        {
            const float a = detector[static_cast<size_t>((detectorWrite + i) % detectorSize)];
            const float b = detector[static_cast<size_t>((detectorWrite + i + lag) % detectorSize)];
            difference += (a - b) * (a - b);
            normA += a * a;
            normB += b * b;
        }
        const float score = static_cast<float>(difference / (normA + normB + 1.0e-12));
        if (score < best) { best = score; bestLag = lag; }
        if (score > previousScore && previousScore < 0.20f)
        {
            best = previousScore;
            bestLag = lag - 2;
            foundVoicedMinimum = true;
            break;
        }
        previousScore = score;
    }

    if (bestLag == 0 || best > 0.32f || (best > 0.24f && !foundVoicedMinimum))
    { detectedMidi = -1.0f; targetPitchRatio = 1.0f; return; }
    const float hz = static_cast<float>(currentSampleRate) / static_cast<float>(bestLag);
    const float measuredMidi = 69.0f + 12.0f * std::log2(hz / 440.0f);
    if (detectedMidi < 0.0f || std::abs(measuredMidi - detectedMidi) > 7.0f)
        detectedMidi = measuredMidi;
    else
        detectedMidi += (measuredMidi - detectedMidi) * 0.35f;
    targetPitchRatio = tunedRatio();
}

float ToneSnapAudioProcessor::tunedRatio() const noexcept
{
    if (detectedMidi < 0.0f) return 1.0f;
    const int key = static_cast<int>(parameters.getRawParameterValue("key")->load());
    const int scale = static_cast<int>(parameters.getRawParameterValue("scale")->load());
    const float amount = parameters.getRawParameterValue("amount")->load();
    const float speed = parameters.getRawParameterValue("speed")->load();
    static constexpr std::array<int, 12> major { 0, 2, 4, 5, 7, 9, 11, -1, -1, -1, -1, -1 };
    static constexpr std::array<int, 12> minor { 0, 2, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1 };
    const int nearestMidi = static_cast<int>(std::lround(detectedMidi));
    float bestMidi = static_cast<float>(nearestMidi);
    float bestDistance = 100.0f;
    for (int offset = -12; offset <= 12; ++offset)
    {
        const int candidate = nearestMidi + offset;
        const int pitchClass = ((candidate - key) % 12 + 12) % 12;
        bool allowed = scale == 0;
        if (scale == 1) allowed = std::find(major.begin(), major.begin() + 7, pitchClass) != major.begin() + 7;
        else if (scale == 2) allowed = std::find(minor.begin(), minor.begin() + 7, pitchClass) != minor.begin() + 7;
        if (allowed && std::abs(static_cast<float>(candidate) - detectedMidi) < bestDistance)
        {
            bestMidi = static_cast<float>(candidate);
            bestDistance = std::abs(bestMidi - detectedMidi);
        }
    }
    const float correctedMidi = detectedMidi + (bestMidi - detectedMidi) * amount;
    const float correction = std::pow(2.0f, (correctedMidi - detectedMidi) / 12.0f);
    const float retuneBlend = 0.08f + speed * 0.72f;
    return 1.0f + (correction - 1.0f) * retuneBlend;
}

void ToneSnapAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(buffer.getNumChannels(), stretchedBuffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    const float mix = parameters.getRawParameterValue("mix")->load();
    for (int i = 0; i < samples; ++i)
    {
        float detectorSample = 0.0f;
        for (int ch = 0; ch < channels; ++ch) detectorSample += buffer.getSample(ch, i);
        detectorSample /= static_cast<float>(juce::jmax(1, channels));
        detector[static_cast<size_t>(detectorWrite)] = detectorSample;
        detectorWrite = (detectorWrite + 1) % detectorSize;
        if (++samplesSinceAnalysis >= 512) { analysePitch(); samplesSinceAnalysis = 0; }
    }

    int offset = 0;
    while (offset < samples)
    {
        if (!stretcherReady)
        {
            const int count = juce::jmin(samples - offset, seekInputLength - startupInputCount);
            for (int ch = 0; ch < channels; ++ch)
                startupBuffer.copyFrom(ch, startupInputCount, buffer, ch, offset, count);
            for (int i = 0; i < count; ++i)
            {
                const int dryRead = (dryDelayWrite - getLatencySamples() + dryDelayLength) % dryDelayLength;
                for (int ch = 0; ch < channels; ++ch)
                {
                    const float input = buffer.getSample(ch, offset + i);
                    dryDelayBuffer.setSample(ch, dryDelayWrite, input);
                    buffer.setSample(ch, offset + i, dryDelayBuffer.getSample(ch, dryRead) * (1.0f - mix));
                }
                dryDelayWrite = (dryDelayWrite + 1) % dryDelayLength;
            }
            startupInputCount += count;
            offset += count;
            if (startupInputCount == seekInputLength)
            {
                std::array<const float*, 2> startPointers{};
                for (int ch = 0; ch < channels; ++ch)
                    startPointers[static_cast<size_t>(ch)] = startupBuffer.getReadPointer(ch);
                smoothedRatio = targetPitchRatio;
                stretcher.setTransposeFactor(smoothedRatio);
                stretcher.outputSeek(startPointers.data(), seekInputLength);
                stretcherReady = true;
            }
            continue;
        }

        const int count = juce::jmin(maximumBlockSize, juce::jmin(64, samples - offset));
        const float speed = parameters.getRawParameterValue("speed")->load();
        const float timeConstant = 0.060f + (1.0f - speed) * 0.140f;
        const float smoothing = 1.0f - std::exp(-static_cast<float>(count) /
                                                 (timeConstant * static_cast<float>(currentSampleRate)));
        smoothedRatio += (targetPitchRatio - smoothedRatio) * smoothing;
        stretcher.setTransposeFactor(smoothedRatio);
        std::array<const float*, 2> inputPointers{};
        std::array<float*, 2> outputPointers{};
        for (int ch = 0; ch < channels; ++ch)
        {
            inputPointers[static_cast<size_t>(ch)] = buffer.getReadPointer(ch, offset);
            outputPointers[static_cast<size_t>(ch)] = stretchedBuffer.getWritePointer(ch);
        }
        stretcher.process(inputPointers.data(), count, outputPointers.data(), count);

        for (int i = 0; i < count; ++i)
        {
            const int dryRead = (dryDelayWrite - getLatencySamples() + dryDelayLength) % dryDelayLength;
            for (int ch = 0; ch < channels; ++ch)
            {
                const float input = buffer.getSample(ch, offset + i);
                dryDelayBuffer.setSample(ch, dryDelayWrite, input);
                const float delayedDry = dryDelayBuffer.getSample(ch, dryRead);
                const float wet = stretchedBuffer.getSample(ch, i);
                buffer.setSample(ch, offset + i, delayedDry + (wet - delayedDry) * mix);
            }
            dryDelayWrite = (dryDelayWrite + 1) % dryDelayLength;
        }
        offset += count;
    }
}

juce::AudioProcessorEditor* ToneSnapAudioProcessor::createEditor()
{
    return new ToneSnapAudioProcessorEditor(*this);
}

void ToneSnapAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState(); auto xml = state.createXml()) copyXmlToBinary(*xml, destData);
}

void ToneSnapAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes); xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ToneSnapAudioProcessor();
}
