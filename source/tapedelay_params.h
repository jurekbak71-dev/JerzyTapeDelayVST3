#pragma once

#include <algorithm>

namespace JerzyAudio {

enum ParameterIds : unsigned int {
    kTimeId = 100,
    kFeedbackId,
    kMixId,
    kDriveId,
    kToneId,
    kWowFlutterId
};

struct NormalizedParams {
    double time = (350.0 - 20.0) / 980.0;
    double feedback = 0.42 / 0.95;
    double mix = 0.35;
    double drive = 6.0 / 24.0;
    double tone = (6500.0 - 1200.0) / (18000.0 - 1200.0);
    double wowFlutter = 0.22;
};

struct PlainParams {
    double timeMs = 350.0;
    double feedback = 0.42;
    double mix = 0.35;
    double driveDb = 6.0;
    double toneHz = 6500.0;
    double wowFlutter = 0.22;
};

inline double clamp01(double v) { return std::clamp(v, 0.0, 1.0); }

inline PlainParams toPlain(const NormalizedParams& n) {
    PlainParams p;
    p.timeMs = 20.0 + clamp01(n.time) * 980.0;
    p.feedback = clamp01(n.feedback) * 0.95;
    p.mix = clamp01(n.mix);
    p.driveDb = clamp01(n.drive) * 24.0;
    p.toneHz = 1200.0 + clamp01(n.tone) * (18000.0 - 1200.0);
    p.wowFlutter = clamp01(n.wowFlutter);
    return p;
}

} // namespace JerzyAudio
