#include "../source/tapedelay_dsp.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

int main() {
    constexpr int sr = 48000;
    constexpr int n = sr * 2;
    std::vector<float> inL(n, 0.f), inR(n, 0.f), outL(n, 0.f), outR(n, 0.f);
    inL[0] = inR[0] = 1.f;
    const float* in[2] = {inL.data(), inR.data()};
    float* out[2] = {outL.data(), outR.data()};

    JerzyAudio::TapeDelayDSP<float> dsp;
    dsp.prepare(sr, 2);
    JerzyAudio::PlainParams p;
    p.timeMs = 250.0;
    p.feedback = 0.45;
    p.mix = 1.0;
    p.driveDb = 6.0;
    p.toneHz = 6500.0;
    p.wowFlutter = 0.0;
    dsp.setParams(p);
    dsp.process(in, out, 2, n);

    const int expected = static_cast<int>(0.250 * sr);
    int peakIndex = 0;
    float peak = 0.f;
    for (int i = expected - 8; i <= expected + 8; ++i) {
        if (std::abs(outL[i]) > peak) { peak = std::abs(outL[i]); peakIndex = i; }
    }
    const float maxAbs = *std::max_element(outL.begin(), outL.end(), [](float a, float b){return std::abs(a) < std::abs(b);});
    if (peak < 0.05f || std::abs(peakIndex - expected) > 2 || !std::isfinite(maxAbs)) {
        std::cerr << "DSP smoke test FAILED\n";
        return 1;
    }
    std::cout << "DSP smoke test OK; first echo near sample " << peakIndex << ", peak=" << peak << "\n";
    return 0;
}
