#pragma once

#include "tapedelay_params.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace JerzyAudio {

template <typename Sample>
class TapeDelayDSP {
public:
    void prepare(double newSampleRate, int channels = 2) {
        sampleRate = std::max(8000.0, newSampleRate);
        numChannels = std::clamp(channels, 1, 2);
        const auto maxSamples = static_cast<std::size_t>(std::ceil(sampleRate * 2.10)) + 8;
        for (auto& b : delayBuffer) b.assign(maxSamples, Sample(0));
        writeIndex = 0;
        wowPhase = 0.0;
        flutterPhase = 0.0;
        filterState[0] = filterState[1] = Sample(0);
    }

    void reset() {
        for (auto& b : delayBuffer) std::fill(b.begin(), b.end(), Sample(0));
        writeIndex = 0;
        filterState[0] = filterState[1] = Sample(0);
        wowPhase = flutterPhase = 0.0;
    }

    void setParams(const PlainParams& p) { params = p; }

    void process(const Sample* const* inputs, Sample* const* outputs, int channels, int numSamples) {
        if (delayBuffer[0].empty() || numSamples <= 0) return;
        channels = std::clamp(channels, 1, numChannels);

        const double cutoff = std::clamp(params.toneHz, 100.0, sampleRate * 0.45);
        const double lpA = std::exp(-2.0 * kPi * cutoff / sampleRate);
        const double lpB = 1.0 - lpA;
        const double driveGain = std::pow(10.0, params.driveDb / 20.0);
        const double satNorm = std::tanh(driveGain);
        const double feedback = std::clamp(params.feedback, 0.0, 0.95);
        const double mix = std::clamp(params.mix, 0.0, 1.0);
        const double wowAmount = std::clamp(params.wowFlutter, 0.0, 1.0);

        for (int i = 0; i < numSamples; ++i) {
            const double wowMs = std::sin(wowPhase) * (4.0 * wowAmount);
            const double flutterMsBase = std::sin(flutterPhase) * (0.65 * wowAmount);

            for (int ch = 0; ch < channels; ++ch) {
                const double stereoPhase = (ch == 0 ? 0.0 : 0.71);
                const double flutterMs = flutterMsBase +
                    std::sin(flutterPhase + stereoPhase) * (0.20 * wowAmount);
                const double delayMs = std::clamp(params.timeMs + wowMs + flutterMs, 1.0, 2000.0);
                const double delaySamples = delayMs * sampleRate / 1000.0;

                const Sample delayed = readInterpolated(ch, delaySamples);
                const Sample in = inputs && inputs[ch] ? inputs[ch][i] : Sample(0);

                const double record = static_cast<double>(in) + static_cast<double>(delayed) * feedback;
                const double filtered = lpB * record + lpA * static_cast<double>(filterState[ch]);
                filterState[ch] = static_cast<Sample>(filtered);
                const double saturated = satNorm > 1.0e-9 ? std::tanh(filtered * driveGain) / satNorm : filtered;
                delayBuffer[ch][writeIndex] = static_cast<Sample>(saturated);

                if (outputs && outputs[ch]) {
                    outputs[ch][i] = static_cast<Sample>(static_cast<double>(in) * (1.0 - mix) +
                                                         static_cast<double>(delayed) * mix);
                }
            }

            writeIndex = (writeIndex + 1) % delayBuffer[0].size();
            wowPhase += 2.0 * kPi * 0.33 / sampleRate;
            flutterPhase += 2.0 * kPi * 5.7 / sampleRate;
            if (wowPhase >= 2.0 * kPi) wowPhase -= 2.0 * kPi;
            if (flutterPhase >= 2.0 * kPi) flutterPhase -= 2.0 * kPi;
        }
    }

private:
    Sample readInterpolated(int channel, double delaySamples) const {
        const auto size = delayBuffer[channel].size();
        double readPos = static_cast<double>(writeIndex) - delaySamples;
        while (readPos < 0.0) readPos += static_cast<double>(size);
        while (readPos >= static_cast<double>(size)) readPos -= static_cast<double>(size);

        const auto i0 = static_cast<std::size_t>(readPos);
        const auto i1 = (i0 + 1) % size;
        const double frac = readPos - static_cast<double>(i0);
        return static_cast<Sample>(static_cast<double>(delayBuffer[channel][i0]) * (1.0 - frac) +
                                   static_cast<double>(delayBuffer[channel][i1]) * frac);
    }

    static constexpr double kPi = 3.1415926535897932384626433832795;
    double sampleRate = 44100.0;
    int numChannels = 2;
    std::vector<Sample> delayBuffer[2];
    std::size_t writeIndex = 0;
    Sample filterState[2] { Sample(0), Sample(0) };
    double wowPhase = 0.0;
    double flutterPhase = 0.0;
    PlainParams params {};
};

} // namespace JerzyAudio
