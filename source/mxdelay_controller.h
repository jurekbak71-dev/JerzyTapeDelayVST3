#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "vstgui/plugin-bindings/vst3editor.h"
namespace JerzyAudio {
class MXDelayController : public Steinberg::Vst::EditControllerEx1, public Steinberg::Vst::IMidiMapping {
public:
    static Steinberg::FUnknown* createInstance(void*){return static_cast<Steinberg::Vst::IEditController*>(new MXDelayController);}
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown*) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream*) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView(const char*) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getMidiControllerAssignment(Steinberg::int32,Steinberg::int16,Steinberg::Vst::CtrlNumber,Steinberg::Vst::ParamID&) SMTG_OVERRIDE;
    DELEGATE_REFCOUNT(Steinberg::Vst::EditController)
    Steinberg::tresult PLUGIN_API queryInterface(const char* iid,void** obj) SMTG_OVERRIDE;
};
}
