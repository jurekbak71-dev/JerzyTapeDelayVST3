#include "tapedelay_controller.h"
#include "tapedelay_params.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace JerzyAudio {

tresult PLUGIN_API TapeDelayController::initialize(FUnknown* context) {
    auto result = EditControllerEx1::initialize(context);
    if (result != kResultOk) return result;

    auto addRange = [this](const TChar* title, ParamID id, const TChar* unit,
                           double lo, double hi, double def, int precision,
                           int32 flags = ParameterInfo::kCanAutomate, int32 steps = 0) {
        auto* p = new RangeParameter(title, id, unit, lo, hi, def, steps, flags);
        p->setPrecision(precision);
        parameters.addParameter(p);
    };

    addRange(STR16("Time"),        kTimeId,       STR16("ms"), 20.0,   1000.0, 350.0, 1);
    addRange(STR16("Feedback"),    kFeedbackId,   STR16("%"),   0.0,     95.0,  42.0, 1);
    addRange(STR16("Mix"),         kMixId,        STR16("%"),   0.0,    100.0,  35.0, 1);
    addRange(STR16("Drive"),       kDriveId,      STR16("dB"),  0.0,     24.0,   6.0, 1);
    addRange(STR16("Tone"),        kToneId,       STR16("Hz"), 1200.0, 18000.0, 6500.0, 0);
    addRange(STR16("Wow/Flutter"), kWowFlutterId, STR16("%"),   0.0,    100.0,  22.0, 1);
    addRange(STR16("Bypass"),      kBypassId,     STR16(""),    0.0,      1.0,   0.0, 0,
             ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, 1);
    addRange(STR16("Output"),      kMeterId,      STR16(""),    0.0,      1.0,   0.0, 2,
             ParameterInfo::kIsReadOnly);

    return kResultOk;
}

tresult PLUGIN_API TapeDelayController::setComponentState(IBStream* state) {
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    float values[6] {};
    for (auto& v : values) if (!s.readFloat(v)) return kResultFalse;

    setParamNormalized(kTimeId, values[0]);
    setParamNormalized(kFeedbackId, values[1]);
    setParamNormalized(kMixId, values[2]);
    setParamNormalized(kDriveId, values[3]);
    setParamNormalized(kToneId, values[4]);
    setParamNormalized(kWowFlutterId, values[5]);

    float bypass = 0.f;
    if (s.readFloat(bypass))
        setParamNormalized(kBypassId, bypass);

    return kResultOk;
}

IPlugView* PLUGIN_API TapeDelayController::createView(const char* name) {
    if (name && std::strcmp(name, ViewType::kEditor) == 0)
        return new VSTGUI::VST3Editor(this, "view", "tapedelay.uidesc");
    return nullptr;
}

} // namespace JerzyAudio
