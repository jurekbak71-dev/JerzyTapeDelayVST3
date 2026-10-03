#pragma once
#include "public.sdk/source/vst/vstguieditor.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/vst/ivstplugview.h"
#include "vstgui/lib/controls/icontrollistener.h"
#include <vector>
namespace JerzyAudio {
class VectorControl; class VectorPanel;
class TapeDriveEditor final : public Steinberg::Vst::VSTGUIEditor,
 public Steinberg::IPlugViewContentScaleSupport,public Steinberg::Vst::IParameterFinder,public VSTGUI::IControlListener {
public:
 explicit TapeDriveEditor(Steinberg::Vst::EditController*);
 ~TapeDriveEditor() override;
 DELEGATE_REFCOUNT(Steinberg::Vst::VSTGUIEditor)
 Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID,void**) override;
 Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect*) override;
 Steinberg::tresult PLUGIN_API canResize() override;
 Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect*) override;
 Steinberg::tresult PLUGIN_API setContentScaleFactor(ScaleFactor) override;
 Steinberg::tresult PLUGIN_API findParameter(Steinberg::int32,Steinberg::int32,Steinberg::Vst::ParamID&) override;
 bool PLUGIN_API open(void*,const VSTGUI::PlatformType&) override;
 void PLUGIN_API close() override;
 void valueChanged(VSTGUI::CControl*) override;
 void beginEdit(int32_t) override;
 void endEdit(int32_t) override;
 VSTGUI::CMessageResult notify(VSTGUI::CBaseObject*,const char*) override;
 bool beforeSizeChange(const VSTGUI::CRect&,const VSTGUI::CRect&) override{return true;}
 int32_t getKnobMode() const override{return VSTGUI::kLinearMode;}
private:
 void layout(int,int);void refresh();bool requestSize(int,int);void constrain(int&,int&,bool screen) const;
 std::vector<VectorControl*> controls;VectorPanel* panel=nullptr;void* nativeParent=nullptr;
 bool sizing=false;double dpi=1.;
};
}
