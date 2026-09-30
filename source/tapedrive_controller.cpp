#include "tapedrive_controller.h"
#include "tapedrive_params.h"
#include "tapedrive_gui_views.h"
#include "tapedrive_editor.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"
#include <cstring>

using namespace Steinberg; using namespace Steinberg::Vst;
namespace JerzyAudio {

tresult PLUGIN_API TapeDriveController::initialize(FUnknown*c){
 registerTapeDriveViews();
 auto r=EditControllerEx1::initialize(c); if(r!=kResultOk)return r;
 auto add=[this](const TChar*n,ParamID id,const TChar*u,double lo,double hi,double def,int prec,int32 steps=0,int32 flags=ParameterInfo::kCanAutomate){
  auto*p=new RangeParameter(n,id,u,lo,hi,def,steps,flags); p->setPrecision(prec); parameters.addParameter(p);
 };
 add(STR16("Tape Sat"),kSatId,STR16("%"),0,100,35,1);
 add(STR16("Level"),kLevelId,STR16("dB"),-24,12,-6,1);
 add(STR16("Dry"),kDryId,STR16("%"),0,100,0,1);
 add(STR16("Gain"),kGainModeId,STR16(""),0,1,0,0,1);
 add(STR16("Shift"),kShiftId,STR16(""),0,2,1,0,2);
 add(STR16("Bypass"),kDriveBypassId,STR16(""),0,1,0,0,1,ParameterInfo::kCanAutomate|ParameterInfo::kIsBypass);
 add(STR16("HPF"),kHPFCutoffId,STR16("Hz"),20,2000,20,0);
 add(STR16("HPF Res"),kHPFResId,STR16("Q"),0.5,12.0,0.707,2);
 add(STR16("LPF"),kLPFCutoffId,STR16("Hz"),1000,20000,20000,0);
 add(STR16("LPF Res"),kLPFResId,STR16("Q"),0.5,12.0,0.707,2);
 add(STR16("Wow Flutter"),kWowFlutterId,STR16("%"),0,100,0,1);
 add(STR16("Preamp Mode"),kPreampModeId,STR16(""),0,2,0,0,2);
 add(STR16("Preamp Drive"),kPreampDriveId,STR16("%"),0,100,20,1);
 add(STR16("Input Meter"),kInputMeterId,STR16(""),0,1,0,2,0,ParameterInfo::kIsReadOnly);
 add(STR16("Saturation Meter"),kSaturationMeterId,STR16(""),0,1,0,2,0,ParameterInfo::kIsReadOnly);
 add(STR16("Output Meter"),kDriveMeterId,STR16(""),0,1,0,2,0,ParameterInfo::kIsReadOnly);
 return kResultOk;
}

tresult PLUGIN_API TapeDriveController::setComponentState(IBStream*s){
 if(!s)return kResultFalse;
 IBStreamer b(s,kLittleEndian);
 float v[13]{};
 for(int i=0;i<6;++i) if(!b.readFloat(v[i])) return kResultFalse;
 setParamNormalized(kSatId,v[0]); setParamNormalized(kLevelId,v[1]); setParamNormalized(kDryId,v[2]);
 setParamNormalized(kGainModeId,v[3]); setParamNormalized(kShiftId,v[4]); setParamNormalized(kDriveBypassId,v[5]);
 if(b.readFloat(v[6])) setParamNormalized(kHPFCutoffId,v[6]);
 if(b.readFloat(v[7])) setParamNormalized(kHPFResId,v[7]);
 if(b.readFloat(v[8])) setParamNormalized(kLPFCutoffId,v[8]);
 if(b.readFloat(v[9])) setParamNormalized(kLPFResId,v[9]);
 if(b.readFloat(v[10])) setParamNormalized(kWowFlutterId,v[10]);
 if(b.readFloat(v[11])) setParamNormalized(kPreampModeId,v[11]);
 if(b.readFloat(v[12])) setParamNormalized(kPreampDriveId,v[12]);
 return kResultOk;
}

IPlugView* PLUGIN_API TapeDriveController::createView(const char*n){
 if(n&&std::strcmp(n,ViewType::kEditor)==0){
   auto* editor = new TapeDriveEditor(this,"view","tapedrive_steel.uidesc");
   editor->setAllowedZoomFactors({0.75,1.0,1.25,1.5});
   return editor;
 }
 return nullptr;
}

}
