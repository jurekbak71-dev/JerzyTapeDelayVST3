#pragma once
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "mxdelay_dsp.h"
#include "mxdelay_params.h"
namespace JerzyAudio {
class MXDelayProcessor : public Steinberg::Vst::AudioEffect {
public:
    MXDelayProcessor();
    static Steinberg::FUnknown* createInstance(void*){ return static_cast<Steinberg::Vst::IAudioProcessor*>(new MXDelayProcessor); }
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement*,Steinberg::int32,Steinberg::Vst::SpeakerArrangement*,Steinberg::int32) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup&) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData&) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream*) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream*) SMTG_OVERRIDE;
    Steinberg::uint32 PLUGIN_API getTailSamples() SMTG_OVERRIDE { return Steinberg::Vst::kInfiniteTail; }
private:
    void readChanges(Steinberg::Vst::IParameterChanges*);
    void setNormalized(Steinberg::Vst::ParamID,double);
    void sendTempo(Steinberg::Vst::ProcessData&);
    MXDelayParams p{};
    MXDelayDSP<float> dsp32; MXDelayDSP<double> dsp64;
    double sampleRate=44100.0,bpm=120.0,lastPeak=0.0;
};
}
