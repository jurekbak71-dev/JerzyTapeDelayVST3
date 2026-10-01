#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
const juce::StringArray noteNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
const std::array<const char*, 12> noteParameterIds {
    "noteC", "noteCs", "noteD", "noteDs", "noteE", "noteF",
    "noteFs", "noteG", "noteGs", "noteA", "noteAs", "noteB"
};
const juce::StringArray scaleNames { "Chromatic", "Major", "Minor" };

std::array<float, 5> makeEqCoefficients(double sampleRate, float frequency, float gainDb, int type)
{
    const float safeFrequency = juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.42), frequency);
    const float a = std::pow(10.0f, gainDb / 40.0f);
    const float omega = juce::MathConstants<float>::twoPi * safeFrequency / static_cast<float>(sampleRate);
    const float cosine = std::cos(omega);
    const float sine = std::sin(omega);
    const float alpha = 0.5f * sine * std::sqrt(2.0f);
    const float beta = 2.0f * std::sqrt(a) * alpha;
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
    if (type == 0)
    {
        b0 = a * ((a + 1.0f) - (a - 1.0f) * cosine + beta);
        b1 = 2.0f * a * ((a - 1.0f) - (a + 1.0f) * cosine);
        b2 = a * ((a + 1.0f) - (a - 1.0f) * cosine - beta);
        a0 = (a + 1.0f) + (a - 1.0f) * cosine + beta;
        a1 = -2.0f * ((a - 1.0f) + (a + 1.0f) * cosine);
        a2 = (a + 1.0f) + (a - 1.0f) * cosine - beta;
    }
    else if (type == 1)
    {
        constexpr float q = 0.8f;
        const float peakAlpha = sine / (2.0f * q);
        b0 = 1.0f + peakAlpha * a; b1 = -2.0f * cosine; b2 = 1.0f - peakAlpha * a;
        a0 = 1.0f + peakAlpha / a; a1 = -2.0f * cosine; a2 = 1.0f - peakAlpha / a;
    }
    else
    {
        b0 = a * ((a + 1.0f) + (a - 1.0f) * cosine + beta);
        b1 = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosine);
        b2 = a * ((a + 1.0f) + (a - 1.0f) * cosine - beta);
        a0 = (a + 1.0f) - (a - 1.0f) * cosine + beta;
        a1 = 2.0f * ((a - 1.0f) - (a + 1.0f) * cosine);
        a2 = (a + 1.0f) - (a - 1.0f) * cosine - beta;
    }
    const float invA0 = 1.0f / a0;
    return { b0 * invA0, b1 * invA0, b2 * invA0, a1 * invA0, a2 * invA0 };
}
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
    for (size_t i = 0; i < noteNames.size(); ++i)
        layout.add(std::make_unique<juce::AudioParameterBool>(noteParameterIds[i], noteNames[i], true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("speed", "Speed", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f, "ms"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("amount", "Amount", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterBool>("compEnabled", "Compressor", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("compThreshold", "Comp Threshold", juce::NormalisableRange<float>(-36.0f, 0.0f, 0.1f), -18.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("compRatio", "Comp Ratio", juce::NormalisableRange<float>(1.0f, 12.0f, 0.1f), 3.0f, ":1"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("compMakeup", "Comp Makeup", juce::NormalisableRange<float>(-6.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterBool>("eqEnabled", "Equalizer", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqLow", "EQ Low", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqMid", "EQ Mid", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqHigh", "EQ High", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f, "dB"));
    return layout;
}

float ToneSnapAudioProcessor::Biquad::process(float input, float coefficientSmoothing) noexcept
{
    for (size_t i = 0; i < coefficients.size(); ++i)
        coefficients[i] += (targetCoefficients[i] - coefficients[i]) * coefficientSmoothing;
    const float output = coefficients[0] * input + coefficients[1] * x1 + coefficients[2] * x2
                       - coefficients[3] * y1 - coefficients[4] * y2;
    x2 = x1; x1 = input; y2 = y1; y1 = output;
    return output;
}

void ToneSnapAudioProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    currentSampleRate = sampleRate;
    detector.fill(0.0f);
    detectorWrite = samplesSinceAnalysis = 0;
    detectedMidi = -1.0f;
    targetPitchRatio = 1.0f;
    smoothedRatio = 1.0f;
    const auto initial = [this](const char* id) { return parameters.getRawParameterValue(id)->load(); };
    const auto initialiseSmoother = [sampleRate](juce::SmoothedValue<float>& smoother, float value, double rampSeconds)
    {
        smoother.reset(sampleRate, rampSeconds);
        smoother.setCurrentAndTargetValue(value);
    };
    initialiseSmoother(mixSmoother, initial("mix") * 0.01f, 0.01);
    initialiseSmoother(speedSmoother, initial("speed"), 0.01);
    initialiseSmoother(compThresholdSmoother, initial("compThreshold"), 0.02);
    initialiseSmoother(compRatioSmoother, initial("compRatio"), 0.02);
    initialiseSmoother(compMakeupSmoother, initial("compMakeup"), 0.02);
    initialiseSmoother(compBlendSmoother, initial("compEnabled"), 0.02);
    initialiseSmoother(eqBlendSmoother, initial("eqEnabled"), 0.02);
    initialiseSmoother(outputGainSmoother, initial("outputGain"), 0.02);
    compressorEnvelope.fill(0.0f);
    inputPeak.store(0.0f, std::memory_order_relaxed);
    outputPeak.store(0.0f, std::memory_order_relaxed);
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
    for (auto& channel : eqFilters)
        for (auto& filter : channel) filter.reset();
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
    // Full one-sample lag scan with sub-sample parabolic refinement.
    // The previous 2-sample grid was too coarse for reliable note decisions.
    constexpr int minLag = 44;
    constexpr int maxLag = 850;
    std::array<float, maxLag + 1> scores{};
    scores.fill(1.0f);
    double energy = 0.0;
    for (int i = 0; i < detectorSize; ++i) energy += detector[static_cast<size_t>(i)] * detector[static_cast<size_t>(i)];
    if (energy / detectorSize < 0.000002) { detectedMidi = -1.0f; targetPitchRatio = 1.0f; return; }

    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double difference = 0.0, normA = 0.0, normB = 0.0;
        for (int i = 0; i < detectorSize - maxLag; i += 8)
        {
            const float a = detector[static_cast<size_t>((detectorWrite + i) % detectorSize)];
            const float b = detector[static_cast<size_t>((detectorWrite + i + lag) % detectorSize)];
            difference += (a - b) * (a - b);
            normA += a * a;
            normB += b * b;
        }
        scores[static_cast<size_t>(lag)] = static_cast<float>(difference / (normA + normB + 1.0e-12));
    }

    int bestLag = 0;
    float bestScore = 1.0f;
    for (int lag = minLag + 1; lag < maxLag; ++lag)
    {
        const float score = scores[static_cast<size_t>(lag)];
        if (score <= scores[static_cast<size_t>(lag - 1)] && score < scores[static_cast<size_t>(lag + 1)] && score < 0.24f)
        {
            bestLag = lag;
            bestScore = score;
            break; // First confident YIN minimum favours the fundamental over its harmonics.
        }
        if (score < bestScore) { bestScore = score; bestLag = lag; }
    }
    if (bestLag == 0 || bestScore > 0.32f)
    { detectedMidi = -1.0f; targetPitchRatio = 1.0f; return; }

    const float left = scores[static_cast<size_t>(bestLag - 1)];
    const float centre = scores[static_cast<size_t>(bestLag)];
    const float right = scores[static_cast<size_t>(bestLag + 1)];
    const float curvature = left - 2.0f * centre + right;
    const float fractionalOffset = std::abs(curvature) > 1.0e-9f
        ? juce::jlimit(-0.5f, 0.5f, 0.5f * (left - right) / curvature) : 0.0f;
    const float period = static_cast<float>(bestLag) + fractionalOffset;
    const float hz = static_cast<float>(currentSampleRate) / period;
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
    const float amount = parameters.getRawParameterValue("amount")->load() * 0.01f;
    std::array<bool, 12> enabledNotes {};
    bool anyEnabledNote = false;
    for (size_t i = 0; i < enabledNotes.size(); ++i)
    {
        enabledNotes[i] = parameters.getRawParameterValue(noteParameterIds[i])->load() >= 0.5f;
        anyEnabledNote = anyEnabledNote || enabledNotes[i];
    }
    if (!anyEnabledNote) return 1.0f;
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
        allowed = allowed && enabledNotes[static_cast<size_t>((candidate % 12 + 12) % 12)];
        if (allowed && std::abs(static_cast<float>(candidate) - detectedMidi) < bestDistance)
        {
            bestMidi = static_cast<float>(candidate);
            bestDistance = std::abs(bestMidi - detectedMidi);
        }
    }
    const float correctedMidi = detectedMidi + (bestMidi - detectedMidi) * amount;
    const float correction = std::pow(2.0f, (correctedMidi - detectedMidi) / 12.0f);
    return correction;
}

void ToneSnapAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(buffer.getNumChannels(), stretchedBuffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    const float mixTarget = parameters.getRawParameterValue("mix")->load() * 0.01f;
    const float speedTarget = parameters.getRawParameterValue("speed")->load();
    const float eqLowTarget = parameters.getRawParameterValue("eqLow")->load();
    const float eqMidTarget = parameters.getRawParameterValue("eqMid")->load();
    const float eqHighTarget = parameters.getRawParameterValue("eqHigh")->load();
    mixSmoother.setTargetValue(mixTarget);
    speedSmoother.setTargetValue(speedTarget);
    compThresholdSmoother.setTargetValue(parameters.getRawParameterValue("compThreshold")->load());
    compRatioSmoother.setTargetValue(parameters.getRawParameterValue("compRatio")->load());
    compMakeupSmoother.setTargetValue(parameters.getRawParameterValue("compMakeup")->load());
    compBlendSmoother.setTargetValue(parameters.getRawParameterValue("compEnabled")->load());
    eqBlendSmoother.setTargetValue(parameters.getRawParameterValue("eqEnabled")->load());
    outputGainSmoother.setTargetValue(parameters.getRawParameterValue("outputGain")->load());
    for (int ch = 0; ch < channels; ++ch)
    {
        eqFilters[static_cast<size_t>(ch)][0].setTarget(makeEqCoefficients(currentSampleRate, 120.0f, eqLowTarget, 0));
        eqFilters[static_cast<size_t>(ch)][1].setTarget(makeEqCoefficients(currentSampleRate, 1200.0f, eqMidTarget, 1));
        eqFilters[static_cast<size_t>(ch)][2].setTarget(makeEqCoefficients(currentSampleRate, 8000.0f, eqHighTarget, 2));
    }
    const float coefficientSmoothing = 1.0f - std::exp(-1.0f / (0.02f * static_cast<float>(currentSampleRate)));
    const float attackCoefficient = std::exp(-1.0f / (0.010f * static_cast<float>(currentSampleRate)));
    const float releaseCoefficient = std::exp(-1.0f / (0.120f * static_cast<float>(currentSampleRate)));
    float blockInputPeak = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        float detectorSample = 0.0f;
        for (int ch = 0; ch < channels; ++ch) detectorSample += buffer.getSample(ch, i);
        detectorSample /= static_cast<float>(juce::jmax(1, channels));
        blockInputPeak = juce::jmax(blockInputPeak, std::abs(detectorSample));
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
                const float mixValue = mixSmoother.getNextValue();
                for (int ch = 0; ch < channels; ++ch)
                {
                    const float input = buffer.getSample(ch, offset + i);
                    dryDelayBuffer.setSample(ch, dryDelayWrite, input);
                    buffer.setSample(ch, offset + i, dryDelayBuffer.getSample(ch, dryRead) * (1.0f - mixValue));
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
        const float speedMs = speedSmoother.skip(count);
        const float timeConstant = juce::jmax(0.002f, speedMs * 0.001f);
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
            const float mixValue = mixSmoother.getNextValue();
            for (int ch = 0; ch < channels; ++ch)
            {
                const float input = buffer.getSample(ch, offset + i);
                dryDelayBuffer.setSample(ch, dryDelayWrite, input);
                const float delayedDry = dryDelayBuffer.getSample(ch, dryRead);
                const float wet = stretchedBuffer.getSample(ch, i);
                buffer.setSample(ch, offset + i, delayedDry + (wet - delayedDry) * mixValue);
            }
            dryDelayWrite = (dryDelayWrite + 1) % dryDelayLength;
        }
        offset += count;
    }

    float blockOutputPeak = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        float linkedPeak = 0.0f;
        for (int ch = 0; ch < channels; ++ch)
            linkedPeak = juce::jmax(linkedPeak, std::abs(buffer.getSample(ch, i)));
        const float envelopeCoefficient = linkedPeak > compressorEnvelope[0] ? attackCoefficient : releaseCoefficient;
        compressorEnvelope[0] = envelopeCoefficient * compressorEnvelope[0] + (1.0f - envelopeCoefficient) * linkedPeak;
        const float envelopeDb = juce::Decibels::gainToDecibels(juce::jmax(compressorEnvelope[0], 1.0e-7f));
        const float threshold = compThresholdSmoother.getNextValue();
        const float ratio = juce::jmax(1.0f, compRatioSmoother.getNextValue());
        const float over = envelopeDb - threshold;
        constexpr float kneeDb = 6.0f;
        float reductionDb = 0.0f;
        if (over >= kneeDb * 0.5f)
            reductionDb = over * (1.0f / ratio - 1.0f);
        else if (over > -kneeDb * 0.5f)
        {
            const float kneePosition = over + kneeDb * 0.5f;
            reductionDb = (1.0f / ratio - 1.0f) * kneePosition * kneePosition / (2.0f * kneeDb);
        }
        const float compGain = juce::Decibels::decibelsToGain(reductionDb + compMakeupSmoother.getNextValue());
        const float compBlend = compBlendSmoother.getNextValue();
        const float eqBlend = eqBlendSmoother.getNextValue();
        const float outputGain = juce::Decibels::decibelsToGain(outputGainSmoother.getNextValue());
        for (int ch = 0; ch < channels; ++ch)
        {
            float sample = buffer.getSample(ch, i);
            const float compressed = sample * compGain;
            sample += (compressed - sample) * compBlend;
            float equalized = sample;
            auto& filters = eqFilters[static_cast<size_t>(ch)];
            for (auto& filter : filters) equalized = filter.process(equalized, coefficientSmoothing);
            sample += (equalized - sample) * eqBlend;
            sample *= outputGain;
            buffer.setSample(ch, i, sample);
            blockOutputPeak = juce::jmax(blockOutputPeak, std::abs(sample));
        }
    }
    const float inputMeter = juce::jmax(blockInputPeak, inputPeak.load(std::memory_order_relaxed) * 0.90f);
    const float outputMeter = juce::jmax(blockOutputPeak, outputPeak.load(std::memory_order_relaxed) * 0.90f);
    inputPeak.store(inputMeter, std::memory_order_relaxed);
    outputPeak.store(outputMeter, std::memory_order_relaxed);
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
