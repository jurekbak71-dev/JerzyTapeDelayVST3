#include "tapedrive_editor.h"
#include "tapedrive_gui_views.h"
#include "tapedrive_params.h"
#include "vstgui/lib/cframe.h"
#include "vstgui/lib/platform/iplatformframe.h"
#include "vstgui/lib/cvstguitimer.h"
#include "base/source/fstring.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
using namespace VSTGUI;using namespace Steinberg;using namespace Steinberg::Vst;
namespace JerzyAudio {
namespace {constexpr int baseW=880,baseH=560;constexpr int zoom70=9001,zoom85=9002,zoom100=9003,zoom120=9004,gripTag=9010;}
TapeDriveEditor::TapeDriveEditor(EditController*c):VSTGUIEditor(c){setRect({0,0,baseW,baseH});setIdleRate(33);}
TapeDriveEditor::~TapeDriveEditor(){close();}
tresult PLUGIN_API TapeDriveEditor::queryInterface(const TUID id,void**out){QUERY_INTERFACE(id,out,IPlugViewContentScaleSupport::iid,IPlugViewContentScaleSupport);QUERY_INTERFACE(id,out,IParameterFinder::iid,IParameterFinder);return VSTGUIEditor::queryInterface(id,out);}
void TapeDriveEditor::constrain(int&w,int&h,bool screen)const{
 double z=std::clamp(w/double(baseW),.7,1.6);
#if defined(_WIN32)
 if(screen){RECT work{};HMONITOR m=MonitorFromWindow(static_cast<HWND>(nativeParent),MONITOR_DEFAULTTONEAREST);MONITORINFO info{sizeof(info)};if(GetMonitorInfoW(m,&info))work=info.rcWork;else SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);if(work.right>work.left && work.bottom>work.top)z=std::min(z,std::min((work.right-work.left-80.)/baseW,(work.bottom-work.top-120.)/baseH));}
#endif
 w=int(std::lround(baseW*z));h=int(std::lround(baseH*z));
}
bool PLUGIN_API TapeDriveEditor::open(void*parent,const PlatformType&type){
 nativeParent=parent;int w=getRect().getWidth(),h=getRect().getHeight();constrain(w,h,true);setRect({0,0,w,h});
 frame=new CFrame({0,0,double(w),double(h)},this);frame->setAutosizingEnabled(false);frame->setBackgroundColor(CColor(24,28,33));
 panel=new VectorPanel({0,0,double(w),double(h)});frame->addView(panel);
 auto add=[&](int tag,VectorControl::Kind kind,const char*label,double x,double y,double cw,double ch,int steps=0){auto*c=new VectorControl({x,y,x+cw,y+ch},this,tag,kind,label,steps);if(tag<9000){auto*p=controller->getParameterObject(tag);if(p)c->setDefaultValue(float(p->getInfo().defaultNormalizedValue));}controls.push_back(c);frame->addView(c);};
 add(zoom70,VectorControl::Action,"70%",542,18,68,32);add(zoom85,VectorControl::Action,"85%",620,18,68,32);add(zoom100,VectorControl::Action,"100%",698,18,68,32);add(zoom120,VectorControl::Action,"120%",776,18,68,32);
 add(kOptoAmountId,VectorControl::Knob,"REDUCTION",26,112,80,86);add(kOptoColorId,VectorControl::Knob,"COLOUR",112,112,80,86);add(kOptoMakeupId,VectorControl::Knob,"MAKEUP",198,112,80,86);
 add(kOptoRecoveryId,VectorControl::Knob,"RECOVERY",26,200,80,76);add(kOptoMixId,VectorControl::Knob,"MIX",112,200,80,76);
 add(kOptoBypassId,VectorControl::Choice,"COMP",198,210,80,24,1);add(kOptoMeterId,VectorControl::Meter,"GR",198,241,80,32);
 add(kSatId,VectorControl::Knob,"DRIVE",318,116,112,122);add(kPreampDriveId,VectorControl::Knob,"PREAMP DRIVE",446,116,112,122);
 add(kPreampModeId,VectorControl::Choice,"PREAMP",316,246,142,26,2);add(kGainModeId,VectorControl::Choice,"GAIN",466,246,98,26,1);
 add(kLevelId,VectorControl::Knob,"LEVEL",606,116,112,122);add(kDryId,VectorControl::Knob,"DRY",734,116,112,122);add(kDriveBypassId,VectorControl::Choice,"TAPE DRIVE",606,246,240,26,1);
 add(kWowId,VectorControl::Knob,"WOW",30,332,116,117);add(kFlutterId,VectorControl::Knob,"FLUTTER",164,332,116,117);add(kTapeAgeId,VectorControl::Knob,"AGE",298,332,116,117);
 add(kHPFCutoffId,VectorControl::Knob,"HPF",456,332,92,108);add(kHPFResId,VectorControl::Knob,"HPF Q",558,332,92,108);add(kLPFCutoffId,VectorControl::Knob,"LPF",660,332,92,108);add(kLPFResId,VectorControl::Knob,"LPF Q",762,332,92,108);add(kShiftId,VectorControl::Choice,"CONTOUR",458,447,394,21,2);
 add(kInputMeterId,VectorControl::Meter,"INPUT",28,494,244,44);add(kDriveMeterId,VectorControl::Meter,"OUTPUT",316,494,244,44);add(kSaturationMeterId,VectorControl::Meter,"SATURATION",604,494,238,44);add(gripTag,VectorControl::Grip,"",856,536,20,20);
 layout(w,h);refresh();if(!frame->open(parent,type)){close();return false;}
 // Tell the host about a clamped restored size before the native-size timer runs.
 if(plugFrame)requestSize(w,h);return true;
}
void PLUGIN_API TapeDriveEditor::close(){controls.clear();panel=nullptr;nativeParent=nullptr;if(frame){frame->close();frame=nullptr;}}
void TapeDriveEditor::layout(int w,int h){if(!frame)return;sizing=true;frame->setSize(w,h);panel->setViewSize({0,0,double(w),double(h)});for(auto*c:controls){const auto d=c->design;CRect r(d.left*w/baseW,d.top*h/baseH,d.right*w/baseW,d.bottom*h/baseH);c->setViewSize(r);c->setMouseableArea(r);}frame->invalid();sizing=false;}
tresult PLUGIN_API TapeDriveEditor::onSize(ViewRect*r){if(!r||r->getWidth()<=0||r->getHeight()<=0)return kInvalidArgument;setRect(*r);layout(r->getWidth(),r->getHeight());return kResultTrue;}
tresult PLUGIN_API TapeDriveEditor::canResize(){return kResultTrue;}
tresult PLUGIN_API TapeDriveEditor::checkSizeConstraint(ViewRect*r){if(!r)return kInvalidArgument;int w=r->getWidth(),h=r->getHeight();constrain(w,h,true);r->right=r->left+w;r->bottom=r->top+h;return kResultTrue;}
tresult PLUGIN_API TapeDriveEditor::setContentScaleFactor(ScaleFactor factor){if(!std::isfinite(factor)||factor<=0)return kInvalidArgument;dpi=factor;return kResultTrue;}
bool TapeDriveEditor::requestSize(int w,int h){constrain(w,h,true);if(!plugFrame)return false;ViewRect r(0,0,w,h);return plugFrame->resizeView(this,&r)==kResultTrue;}
void TapeDriveEditor::beginEdit(int32_t tag){if(tag<9000)VSTGUIEditor::beginEdit(tag);}
void TapeDriveEditor::endEdit(int32_t tag){if(tag<9000)VSTGUIEditor::endEdit(tag);}
void TapeDriveEditor::valueChanged(CControl*c){int tag=c->getTag();if(tag>=zoom70&&tag<=zoom120){const double z[]={.7,.85,1.,1.2};requestSize(int(baseW*z[tag-zoom70]),int(baseH*z[tag-zoom70]));return;}if(tag==gripTag){auto*g=static_cast<VectorControl*>(c);requestSize(int(g->anchor.x+10),int(g->anchor.y+10));return;}if(tag>=9000)return;controller->setParamNormalized(tag,c->getValueNormalized());controller->performEdit(tag,c->getValueNormalized());refresh();}
void TapeDriveEditor::refresh(){for(auto*c:controls){int tag=c->getTag();if(tag>=9000||c->isEditing())continue;const auto value=controller->getParamNormalized(tag);if(std::abs(value-c->getValue())>1e-6){c->setValue(float(value));c->invalid();}
 std::string s;char b[64]{};
 if(tag==kPreampModeId){const char*names[]={"OFF","TUBE","TRANSISTOR"};s=names[std::clamp(int(std::lround(value*2)),0,2)];}
 else if(tag==kShiftId){const char*names[]={"FLAT","HIGH","NORMAL"};s=names[std::clamp(int(std::lround(value*2)),0,2)];}
 else if(tag==kGainModeId)s=value>.5?"HIGH":"NORMAL";
 else if(tag==kDriveBypassId||tag==kOptoBypassId)s=value>.5?"BYPASS":"ON";
 else if(tag==kInputMeterId||tag==kDriveMeterId){std::snprintf(b,sizeof(b),"%.1f dB",(20/.35)*std::log10(std::max(value,1e-6)));s=b;}
 else if(tag==kSaturationMeterId){std::snprintf(b,sizeof(b),"%.0f %%",value*100);s=b;}
 else{String128 str{};controller->getParamStringByValue(tag,value,str);Steinberg::String converted(str);converted.toMultiByte(kCP_Utf8);s=converted.text8();if(tag==kLevelId||tag==kOptoMakeupId||tag==kOptoMeterId)s+=" dB";else if(tag==kHPFCutoffId||tag==kLPFCutoffId)s+=" Hz";else if(tag!=kHPFResId&&tag!=kLPFResId)s+=" %";}
 c->setText(s);
}}
CMessageResult TapeDriveEditor::notify(CBaseObject*sender,const char*message){if(message==CVSTGUITimer::kMsgTimer){
#if defined(_WIN32)
 if(frame && nativeParent && !sizing){RECT r{};if(GetClientRect(static_cast<HWND>(nativeParent),&r)&&r.right>0&&r.bottom>0&&(r.right!=getRect().getWidth()||r.bottom!=getRect().getHeight())){ViewRect size(0,0,r.right,r.bottom);onSize(&size);}}
#endif
 refresh();}return VSTGUIEditor::notify(sender,message);}
tresult PLUGIN_API TapeDriveEditor::findParameter(int32 x,int32 y,ParamID&id){for(auto*c:controls)if(c->getTag()<9000&&c->kind!=VectorControl::Meter&&c->getViewSize().pointInside({double(x),double(y)})){id=c->getTag();return kResultTrue;}return kResultFalse;}
}
