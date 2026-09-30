#include "tapedelay_processor.h"
#include "tapedelay_ids.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace JerzyAudio {

TapeDelayProcessor::TapeDelayProcessor() {
    setControllerClass(kControllerUID);
}

tresult PLUGIN_API TapeDelayProcessor::initialize(FUnknown* context) {
    auto result = AudioEffect::initialize(context);
    if (result != kResultOk) return result;
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API TapeDelayProcessor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                                           SpeakerArrangement* outputs, int32 numOuts) {
    if (numIns != 1 || numOuts != 1) return kResultFalse;
    const auto inChannels = SpeakerArr::getChannelCount(inputs[0]);
    const auto outChannels = SpeakerArr::getChannelCount(outputs[0]);
    if (inChannels != outChannels || (inChannels != 1 && inChannels != 2)) return kResultFalse;
    return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

tresult PLUGIN_API TapeDelayProcessor::setupProcessing(ProcessSetup& setup) {
    sampleRate = setup.sampleRate > 0.0 ? setup.sampleRate : 44100.0;
    dsp32.prepare(sampleRate, 2);
    dsp64.prepare(sampleRate, 2);
    meterValue = 0.0;
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API TapeDelayProcessor::setActive(TBool state) {
    if (state) {
        dsp32.reset();
        dsp64.reset();
        meterValue = 0.0;
    }
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API TapeDelayProcessor::canProcessSampleSize(int32 symbolicSampleSize) {
    return (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64) ? kResultTrue : kResultFalse;
}

void TapeDelayProcessor::readParameterChanges(IParameterChanges* changes) {
    if (!changes) return;
    const int32 count = changes->getParameterCount();
    for (int32 i = 0; i < count; ++i) {
        if (auto* queue = changes->getParameterData(i)) {
            const int32 points = queue->getPointCount();
            if (points <= 0) continue;
            int32 sampleOffset = 0;
            ParamValue value = 0.0;
            if (queue->getPoint(points - 1, sampleOffset, value) != kResultTrue) continue;
            value = std::clamp(value, 0.0, 1.0);
            switch (queue->getParameterId()) {
                case kTimeId: normalized.time = value; break;
                case kFeedbackId: normalized.feedback = value; break;
                case kMixId: normalized.mix = value; break;
                case kDriveId: normalized.drive = value; break;
                case kToneId: normalized.tone = value; break;
                case kWowFlutterId: normalized.wowFlutter = value; break;
                case kBypassId: normalized.bypass = value; break;
                default: break;
            }
        }
    }
}

void TapeDelayProcessor::sendMeter(ProcessData& data, double peak) {
    peak = std::clamp(peak, 0.0, 1.0);
    const double displayPeak = std::pow(peak, 0.35);
    meterValue = std::max(displayPeak, meterValue * 0.90);

    if (!data.outputParameterChanges) return;
    int32 index = 0;
    if (auto* queue = data.outputParameterChanges->addParameterData(kMeterId, index)) {
        int32 pointIndex = 0;
        queue->addPoint(std::max<int32>(0, data.numSamples - 1), meterValue, pointIndex);
    }
}

tresult PLUGIN_API TapeDelayProcessor::process(ProcessData& data) {
    readParameterChanges(data.inputParameterChanges);
    if (data.numInputs == 0 || data.numOutputs == 0 || data.numSamples <= 0) {
        sendMeter(data, 0.0);
        return kResultOk;
    }

    const PlainParams plain = toPlain(normalized);
    const int channels = std::min(data.inputs[0].numChannels, data.outputs[0].numChannels);
    if (channels <= 0) return kResultOk;

    double peak = 0.0;
    const bool bypassed = normalized.bypass >= 0.5;

    if (data.symbolicSampleSize == kSample32) {
        auto** in = data.inputs[0].channelBuffers32;
        auto** out = data.outputs[0].channelBuffers32;
        if (bypassed) {
            for (int ch = 0; ch < channels; ++ch) {
                if (!in[ch] || !out[ch]) continue;
                if (in[ch] != out[ch])
                    std::memcpy(out[ch], in[ch], sizeof(float) * static_cast<size_t>(data.numSamples));
            }
        } else {
            dsp32.setParams(plain);
            dsp32.process(in, out, channels, data.numSamples);
        }
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < data.numSamples; ++i)
                if (out[ch]) peak = std::max(peak, std::abs(static_cast<double>(out[ch][i])));
    } else if (data.symbolicSampleSize == kSample64) {
        auto** in = data.inputs[0].channelBuffers64;
        auto** out = data.outputs[0].channelBuffers64;
        if (bypassed) {
            for (int ch = 0; ch < channels; ++ch) {
                if (!in[ch] || !out[ch]) continue;
                if (in[ch] != out[ch])
                    std::memcpy(out[ch], in[ch], sizeof(double) * static_cast<size_t>(data.numSamples));
            }
        } else {
            dsp64.setParams(plain);
            dsp64.process(in, out, channels, data.numSamples);
        }
        for (int ch = 0; ch < channels; ++ch)
            for (int i = 0; i < data.numSamples; ++i)
                if (out[ch]) peak = std::max(peak, std::abs(out[ch][i]));
    }

    data.outputs[0].silenceFlags = 0;
    sendMeter(data, peak);
    return kResultOk;
}

tresult PLUGIN_API TapeDelayProcessor::setState(IBStream* state) {
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    float values[6] {};
    for (auto& v : values) if (!s.readFloat(v)) return kResultFalse;
    normalized.time = values[0];
    normalized.feedback = values[1];
    normalized.mix = values[2];
    normalized.drive = values[3];
    normalized.tone = values[4];
    normalized.wowFlutter = values[5];

    float bypass = 0.f;
    if (s.readFloat(bypass))
        normalized.bypass = bypass;

    return kResultOk;
}

tresult PLUGIN_API TapeDelayProcessor::getState(IBStream* state) {
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    const float values[7] = {
        static_cast<float>(normalized.time), static_cast<float>(normalized.feedback),
        static_cast<float>(normalized.mix), static_cast<float>(normalized.drive),
        static_cast<float>(normalized.tone), static_cast<float>(normalized.wowFlutter),
        static_cast<float>(normalized.bypass)
    };
    for (auto v : values) if (!s.writeFloat(v)) return kResultFalse;
    return kResultOk;
}

} // namespace JerzyAudio
