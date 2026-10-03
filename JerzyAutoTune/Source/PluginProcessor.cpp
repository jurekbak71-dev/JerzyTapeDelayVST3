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

JerzyAutoTuneAudioProcessor::JerzyAutoTuneAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{}

JerzyAutoTuneAudioProcessor::APVTS::ParameterLayout JerzyAutoTuneAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterChoice>("key", "Key", noteNames, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("scale", "Scale", scaleNames, 1));
    for (int i = 0; i < noteNames.size(); ++i)
        layout.add(std::make_unique<juce::AudioParameterBool>(noteParameterIds[static_cast<size_t>(i)], noteNames[i], true));

    layout.add(std::make_unique<juce::AudioParameterFloat>("speed", "Speed", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f, "ms"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("amount", "Amount", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("mix", "Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f, "%"));

    layout.add(std::make_unique<juce::AudioParameterBool>("gateEnabled", "Gate", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("gateThreshold", "Gate Threshold", juce::NormalisableRange<float>(-70.0f, -20.0f, 0.1f), -48.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("gateRelease", "Gate Release", juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f), 120.0f, "ms"));

    layout.add(std::make_unique<juce::AudioParameterBool>("noiseEnabled", "Noise Filter", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("noiseThreshold", "Noise Threshold", juce::NormalisableRange<float>(-80.0f, -30.0f, 0.1f), -58.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("noiseReduction", "Noise Reduction", juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 10.0f, "dB"));

    layout.add(std::make_unique<juce::AudioParameterBool>("deEsserEnabled", "De-Esser", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("deEsserFreq", "De-Esser Frequency", juce::NormalisableRange<float>(3500.0f, 10000.0f, 10.0f, 0.5f), 6500.0f, "Hz"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("deEsserAmount", "De-Esser Amount", juce::NormalisableRange<float>(0.0f, 18.0f, 0.1f), 6.0f, "dB"));

    layout.add(std::make_unique<juce::AudioParameterBool>("satEnabled", "Saturation", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("satDrive", "Saturation Drive", juce::NormalisableRange<float>(0.0f, 18.0f, 0.1f), 3.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("satMix", "Saturation Mix", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f, "%"));

    layout.add(std::make_unique<juce::AudioParameterBool>("doublerEnabled", "Doubler", false));
    layout.add(std::make_unique<juce::AudioParameterFloat>("doublerAmount", "Doubler Amount", juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("doublerDelay", "Doubler Delay", juce::NormalisableRange<float>(8.0f, 35.0f, 0.1f), 18.0f, "ms"));

    layout.add(std::make_unique<juce::AudioParameterBool>("compEnabled", "Compressor Limiter", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("compThreshold", "Comp Threshold", juce::NormalisableRange<float>(-36.0f, 0.0f, 0.1f), -18.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("compRatio", "Comp Ratio", juce::NormalisableRange<float>(1.0f, 12.0f, 0.1f), 3.0f, ":1"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("compMakeup", "Comp Makeup", juce::NormalisableRange<float>(-6.0f, 12.0f, 0.1f), 1.5f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("limiterCeiling", "Limiter Ceiling", juce::NormalisableRange<float>(-6.0f, -0.1f, 0.1f), -1.0f, "dB"));

    layout.add(std::make_unique<juce::AudioParameterBool>("eqEnabled", "Vocal EQ", true));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqLow", "Vocal EQ Low", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), -1.5f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqMid", "Vocal EQ Presence", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 1.5f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("eqHigh", "Vocal EQ Air", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 1.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output", juce::NormalisableRange<float>(-18.0f, 12.0f, 0.1f), 0.0f, "dB"));
    return layout;
}

float JerzyAutoTuneAudioProcessor::Biquad::process(float input, float coefficientSmoothing) noexcept
{
    for (size_t i = 0; i < coefficients.size(); ++i)
        coefficients[i] += (targetCoefficients[i] - coefficients[i]) * coefficientSmoothing;
    const float output = coefficients[0] * input + coefficients[1] * x1 + coefficients[2] * x2
                       - coefficients[3] * y1 - coefficients[4] * y2;
    x2 = x1; x1 = input; y2 = y1; y1 = output;
    return output;
}

void JerzyAutoTuneAudioProcessor::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    currentSampleRate = sampleRate;
    detector.fill(0.0f);
    detectorWrite = samplesSinceAnalysis = 0;
    detectedMidi = -1.0f;
    targetPitchRatio = smoothedRatio = 1.0f;

    mixSmoother.reset(sampleRate, 0.01);
    mixSmoother.setCurrentAndTargetValue(parameters.getRawParameterValue("mix")->load() * 0.01f);
    speedSmoother.reset(sampleRate, 0.01);
    speedSmoother.setCurrentAndTargetValue(parameters.getRawParameterValue("speed")->load());

    gateEnvelope.fill(0.0f);
    noiseHpState.fill(0.0f);
    noisePrevInput.fill(0.0f);
    deEssSideState.fill(0.0f);
    deEssPrevInput.fill(0.0f);
    deEssEnvelope = compressorEnvelope = 0.0f;
    doublerPhase = 0.0f;
    doublerWrite = 0;

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

    doublerBufferLength = juce::jmax(64, static_cast<int>(sampleRate * 0.08));
    doublerBuffer.setSize(channels, doublerBufferLength, false, true, true);
    doublerBuffer.clear();

    for (auto& channel : vocalEq)
        for (auto& filter : channel)
            filter.reset();

    inputPeak.store(0.0f, std::memory_order_relaxed);
    outputPeak.store(0.0f, std::memory_order_relaxed);
}

void JerzyAutoTuneAudioProcessor::releaseResources() {}

bool JerzyAutoTuneAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return (input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo()) && input == output;
}

void JerzyAutoTuneAudioProcessor::analysePitch() noexcept
{
    constexpr int minLag = 44;
    constexpr int maxLag = 850;
    std::array<float, maxLag + 1> scores {};
    scores.fill(1.0f);

    double energy = 0.0;
    for (int i = 0; i < detectorSize; ++i)
        energy += detector[static_cast<size_t>(i)] * detector[static_cast<size_t>(i)];
    if (energy / detectorSize < 0.000002)
    {
        detectedMidi = -1.0f;
        targetPitchRatio = 1.0f;
        return;
    }

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
            break;
        }
        if (score < bestScore)
        {
            bestScore = score;
            bestLag = lag;
        }
    }

    if (bestLag == 0 || bestScore > 0.32f)
    {
        detectedMidi = -1.0f;
        targetPitchRatio = 1.0f;
        return;
    }

    const float left = scores[static_cast<size_t>(bestLag - 1)];
    const float centre = scores[static_cast<size_t>(bestLag)];
    const float right = scores[static_cast<size_t>(bestLag + 1)];
    const float curvature = left - 2.0f * centre + right;
    const float fractionalOffset = std::abs(curvature) > 1.0e-9f
        ? juce::jlimit(-0.5f, 0.5f, 0.5f * (left - right) / curvature) : 0.0f;

    const float hz = static_cast<float>(currentSampleRate) / (static_cast<float>(bestLag) + fractionalOffset);
    const float measuredMidi = 69.0f + 12.0f * std::log2(hz / 440.0f);
    if (detectedMidi < 0.0f || std::abs(measuredMidi - detectedMidi) > 7.0f)
        detectedMidi = measuredMidi;
    else
        detectedMidi += (measuredMidi - detectedMidi) * 0.35f;

    targetPitchRatio = tunedRatio();
}

float JerzyAutoTuneAudioProcessor::tunedRatio() const noexcept
{
    if (detectedMidi < 0.0f)
        return 1.0f;

    const int key = static_cast<int>(parameters.getRawParameterValue("key")->load());
    const int scale = static_cast<int>(parameters.getRawParameterValue("scale")->load());
    const float amount = parameters.getRawParameterValue("amount")->load() * 0.01f;

    std::array<bool, 12> enabledNotes {};
    bool anyEnabled = false;
    for (size_t i = 0; i < enabledNotes.size(); ++i)
    {
        enabledNotes[i] = parameters.getRawParameterValue(noteParameterIds[i])->load() >= 0.5f;
        anyEnabled = anyEnabled || enabledNotes[i];
    }
    if (!anyEnabled)
        return 1.0f;

    static constexpr std::array<int, 7> major { 0, 2, 4, 5, 7, 9, 11 };
    static constexpr std::array<int, 7> minor { 0, 2, 3, 5, 7, 8, 10 };

    const int nearest = static_cast<int>(std::lround(detectedMidi));
    float bestMidi = static_cast<float>(nearest);
    float bestDistance = 100.0f;

    for (int offset = -12; offset <= 12; ++offset)
    {
        const int candidate = nearest + offset;
        const int relativeClass = ((candidate - key) % 12 + 12) % 12;
        bool allowed = scale == 0;
        if (scale == 1)
            allowed = std::find(major.begin(), major.end(), relativeClass) != major.end();
        else if (scale == 2)
            allowed = std::find(minor.begin(), minor.end(), relativeClass) != minor.end();

        allowed = allowed && enabledNotes[static_cast<size_t>((candidate % 12 + 12) % 12)];
        const float distance = std::abs(static_cast<float>(candidate) - detectedMidi);
        if (allowed && distance < bestDistance)
        {
            bestDistance = distance;
            bestMidi = static_cast<float>(candidate);
        }
    }

    const float correctedMidi = detectedMidi + (bestMidi - detectedMidi) * amount;
    return std::pow(2.0f, (correctedMidi - detectedMidi) / 12.0f);
}

void JerzyAutoTuneAudioProcessor::processVocalChain(juce::AudioBuffer<float>& buffer) noexcept
{
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    if (channels <= 0 || samples <= 0)
        return;

    const bool gateOn = parameters.getRawParameterValue("gateEnabled")->load() >= 0.5f;
    const bool noiseOn = parameters.getRawParameterValue("noiseEnabled")->load() >= 0.5f;
    const bool deEssOn = parameters.getRawParameterValue("deEsserEnabled")->load() >= 0.5f;
    const bool satOn = parameters.getRawParameterValue("satEnabled")->load() >= 0.5f;
    const bool doublerOn = parameters.getRawParameterValue("doublerEnabled")->load() >= 0.5f;
    const bool compOn = parameters.getRawParameterValue("compEnabled")->load() >= 0.5f;
    const bool eqOn = parameters.getRawParameterValue("eqEnabled")->load() >= 0.5f;

    const float gateThreshold = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("gateThreshold")->load());
    const float gateReleaseMs = parameters.getRawParameterValue("gateRelease")->load();
    const float gateRelease = std::exp(-1.0f / (juce::jmax(1.0f, gateReleaseMs) * 0.001f * static_cast<float>(currentSampleRate)));
    const float gateAttack = std::exp(-1.0f / (0.003f * static_cast<float>(currentSampleRate)));

    const float noiseThreshold = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("noiseThreshold")->load());
    const float noiseReduction = juce::Decibels::decibelsToGain(-parameters.getRawParameterValue("noiseReduction")->load());

    const float deEssFreq = parameters.getRawParameterValue("deEsserFreq")->load();
    const float deEssAmount = parameters.getRawParameterValue("deEsserAmount")->load();
    const float deEssHpCoeff = std::exp(-juce::MathConstants<float>::twoPi * deEssFreq / static_cast<float>(currentSampleRate));
    const float deEssRelease = std::exp(-1.0f / (0.060f * static_cast<float>(currentSampleRate)));

    const float satDrive = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("satDrive")->load());
    const float satMix = parameters.getRawParameterValue("satMix")->load() * 0.01f;

    const float doublerMix = parameters.getRawParameterValue("doublerAmount")->load() * 0.01f;
    const float doublerDelayMs = parameters.getRawParameterValue("doublerDelay")->load();
    const float phaseInc = juce::MathConstants<float>::twoPi * 0.23f / static_cast<float>(currentSampleRate);

    const float compThresholdDb = parameters.getRawParameterValue("compThreshold")->load();
    const float compRatio = juce::jmax(1.0f, parameters.getRawParameterValue("compRatio")->load());
    const float compMakeup = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("compMakeup")->load());
    const float limiterCeiling = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("limiterCeiling")->load());
    const float compAttack = std::exp(-1.0f / (0.008f * static_cast<float>(currentSampleRate)));
    const float compRelease = std::exp(-1.0f / (0.110f * static_cast<float>(currentSampleRate)));

    const float lowGain = parameters.getRawParameterValue("eqLow")->load();
    const float midGain = parameters.getRawParameterValue("eqMid")->load();
    const float highGain = parameters.getRawParameterValue("eqHigh")->load();
    for (int ch = 0; ch < channels; ++ch)
    {
        vocalEq[static_cast<size_t>(ch)][0].setTarget(makeEqCoefficients(currentSampleRate, 140.0f, lowGain, 0));
        vocalEq[static_cast<size_t>(ch)][1].setTarget(makeEqCoefficients(currentSampleRate, 2800.0f, midGain, 1));
        vocalEq[static_cast<size_t>(ch)][2].setTarget(makeEqCoefficients(currentSampleRate, 10500.0f, highGain, 2));
    }
    const float eqSmooth = 1.0f - std::exp(-1.0f / (0.025f * static_cast<float>(currentSampleRate)));
    const float outputGain = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("outputGain")->load());

    for (int i = 0; i < samples; ++i)
    {
        float linkedPeak = 0.0f;

        for (int ch = 0; ch < channels; ++ch)
        {
            float x = buffer.getSample(ch, i);
            const float level = std::abs(x);
            const float envCoeff = level > gateEnvelope[static_cast<size_t>(ch)] ? gateAttack : gateRelease;
            gateEnvelope[static_cast<size_t>(ch)] = envCoeff * gateEnvelope[static_cast<size_t>(ch)] + (1.0f - envCoeff) * level;

            if (gateOn)
            {
                const float e = gateEnvelope[static_cast<size_t>(ch)];
                const float gateGain = juce::jlimit(0.0f, 1.0f, (e - gateThreshold * 0.45f) / juce::jmax(1.0e-6f, gateThreshold * 0.55f));
                x *= gateGain;
            }

            if (noiseOn && gateEnvelope[static_cast<size_t>(ch)] < noiseThreshold)
            {
                const float t = juce::jlimit(0.0f, 1.0f, gateEnvelope[static_cast<size_t>(ch)] / juce::jmax(1.0e-7f, noiseThreshold));
                x *= juce::jmap(t, noiseReduction, 1.0f);
            }

            if (deEssOn)
            {
                const float low = (1.0f - deEssHpCoeff) * x + deEssHpCoeff * deEssSideState[static_cast<size_t>(ch)];
                deEssSideState[static_cast<size_t>(ch)] = low;
                const float high = x - low;
                const float s = std::abs(high);
                deEssEnvelope = juce::jmax(s, deEssEnvelope * deEssRelease);
                const float deEssDb = juce::Decibels::gainToDecibels(juce::jmax(deEssEnvelope, 1.0e-7f));
                const float reduction = juce::jlimit(0.0f, deEssAmount, (deEssDb + 34.0f) * 0.55f);
                const float gain = juce::Decibels::decibelsToGain(-reduction);
                x = low + high * gain;
            }

            if (satOn)
            {
                const float driven = std::tanh(x * satDrive) / juce::jmax(1.0f, std::tanh(satDrive));
                x += (driven - x) * satMix;
            }

            doublerBuffer.setSample(ch, doublerWrite, x);
            if (doublerOn)
            {
                const float phaseOffset = ch == 0 ? 0.0f : juce::MathConstants<float>::pi;
                const float modMs = 2.2f * std::sin(doublerPhase + phaseOffset);
                const int delaySamples = juce::jlimit(1, doublerBufferLength - 2,
                    static_cast<int>((doublerDelayMs + modMs) * 0.001f * static_cast<float>(currentSampleRate)));
                const int read = (doublerWrite - delaySamples + doublerBufferLength) % doublerBufferLength;
                const float doubled = doublerBuffer.getSample(ch, read);
                x = x * (1.0f - doublerMix * 0.35f) + doubled * doublerMix * 0.55f;
            }

            buffer.setSample(ch, i, x);
            linkedPeak = juce::jmax(linkedPeak, std::abs(x));
        }

        doublerWrite = (doublerWrite + 1) % doublerBufferLength;
        doublerPhase += phaseInc;
        if (doublerPhase > juce::MathConstants<float>::twoPi)
            doublerPhase -= juce::MathConstants<float>::twoPi;

        const float envCoeff = linkedPeak > compressorEnvelope ? compAttack : compRelease;
        compressorEnvelope = envCoeff * compressorEnvelope + (1.0f - envCoeff) * linkedPeak;
        const float envDb = juce::Decibels::gainToDecibels(juce::jmax(compressorEnvelope, 1.0e-7f));
        float compGain = 1.0f;
        if (compOn && envDb > compThresholdDb)
        {
            const float compressedDb = compThresholdDb + (envDb - compThresholdDb) / compRatio;
            compGain = juce::Decibels::decibelsToGain(compressedDb - envDb) * compMakeup;
        }
        else if (compOn)
            compGain = compMakeup;

        for (int ch = 0; ch < channels; ++ch)
        {
            float x = buffer.getSample(ch, i) * compGain;
            if (compOn)
                x = limiterCeiling * std::tanh(x / juce::jmax(0.05f, limiterCeiling));

            if (eqOn)
            {
                x = vocalEq[static_cast<size_t>(ch)][0].process(x, eqSmooth);
                x = vocalEq[static_cast<size_t>(ch)][1].process(x, eqSmooth);
                x = vocalEq[static_cast<size_t>(ch)][2].process(x, eqSmooth);
            }

            buffer.setSample(ch, i, x * outputGain);
        }
    }
}

void JerzyAutoTuneAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(buffer.getNumChannels(), stretchedBuffer.getNumChannels());
    const int samples = buffer.getNumSamples();

    float blockInputPeak = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        float detectorSample = 0.0f;
        for (int ch = 0; ch < channels; ++ch)
            detectorSample += buffer.getSample(ch, i);
        detectorSample /= static_cast<float>(juce::jmax(1, channels));
        blockInputPeak = juce::jmax(blockInputPeak, std::abs(detectorSample));
        detector[static_cast<size_t>(detectorWrite)] = detectorSample;
        detectorWrite = (detectorWrite + 1) % detectorSize;
        if (++samplesSinceAnalysis >= 512)
        {
            analysePitch();
            samplesSinceAnalysis = 0;
        }
    }

    mixSmoother.setTargetValue(parameters.getRawParameterValue("mix")->load() * 0.01f);
    speedSmoother.setTargetValue(parameters.getRawParameterValue("speed")->load());

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
                std::array<const float*, 2> startPointers {};
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

        std::array<const float*, 2> inputPointers {};
        std::array<float*, 2> outputPointers {};
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
                const float dry = dryDelayBuffer.getSample(ch, dryRead);
                const float wet = stretchedBuffer.getSample(ch, i);
                buffer.setSample(ch, offset + i, dry + (wet - dry) * mixValue);
            }
            dryDelayWrite = (dryDelayWrite + 1) % dryDelayLength;
        }
        offset += count;
    }

    processVocalChain(buffer);

    float blockOutputPeak = 0.0f;
    for (int ch = 0; ch < channels; ++ch)
        for (int i = 0; i < samples; ++i)
            blockOutputPeak = juce::jmax(blockOutputPeak, std::abs(buffer.getSample(ch, i)));

    inputPeak.store(blockInputPeak, std::memory_order_relaxed);
    outputPeak.store(blockOutputPeak, std::memory_order_relaxed);
}

juce::AudioProcessorEditor* JerzyAutoTuneAudioProcessor::createEditor()
{
    return new JerzyAutoTuneAudioProcessorEditor(*this);
}

void JerzyAutoTuneAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void JerzyAutoTuneAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JerzyAutoTuneAudioProcessor();
}
