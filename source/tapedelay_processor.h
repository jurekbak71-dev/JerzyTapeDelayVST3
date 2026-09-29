#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "tapedelay_dsp.h"
#include "tapedelay_params.h"

namespace JerzyAudio {

class TapeDelayProcessor : public Steinberg::Vst::AudioEffect {
public:
    TapeDelayProcessor();
    static Steinberg::FUnknown* createInstance(void*) { return static_cast<Steinberg::Vst::IAudioProcessor*>(new TapeDelayProcessor); }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs,
                                                     Steinberg::int32 numIns,
                                                     Steinberg::Vst::SpeakerArrangement* outputs,
                                                     Steinberg::int32 numOuts) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) SMTG_OVERRIDE;

private:
    void readParameterChanges(Steinberg::Vst::IParameterChanges* changes);
    NormalizedParams normalized {};
    TapeDelayDSP<float> dsp32;
    TapeDelayDSP<double> dsp64;
    double sampleRate = 44100.0;
};

} // namespace JerzyAudio
