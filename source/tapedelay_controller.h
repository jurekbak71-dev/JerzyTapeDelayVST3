#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace JerzyAudio {

class TapeDelayController : public Steinberg::Vst::EditControllerEx1 {
public:
    static Steinberg::FUnknown* createInstance(void*) { return static_cast<Steinberg::Vst::IEditController*>(new TapeDelayController); }
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) SMTG_OVERRIDE;
};

} // namespace JerzyAudio
