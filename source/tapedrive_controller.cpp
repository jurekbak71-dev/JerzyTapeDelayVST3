#include "tapedrive_controller.h"
#include "tapedrive_params.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"
using namespace Steinberg; using namespace Steinberg::Vst;
namespace JerzyAudio {
tresult PLUGIN_API TapeDriveController::initialize(FUnknown*c){
 auto r=EditControllerEx1::initialize(c); if(r!=kResultOk)return r;
 auto add=[this](const TChar*n,ParamID id,const TChar*u,double lo,double hi,double def,int prec,int32 steps=0,int32 flags=ParameterInfo::kCanAutomate){
  auto*p=new RangeParameter(n,id,u,lo,hi,def,steps,flags);p->setPrecision(prec);parameters.addParameter(p);};
 add(STR16("Tape Sat"),kSatId,STR16("%"),0,100,35,1);
 add(STR16("Level"),kLevelId,STR16("dB"),-24,12,-6,1);
 add(STR16("Dry"),kDryId,STR16("%"),0,100,0,1);
 add(STR16("Gain"),kGainModeId,STR16(""),0,1,0,0,1);
 add(STR16("Shift"),kShiftId,STR16(""),0,2,1,0,2);
 add(STR16("Bypass"),kDriveBypassId,STR16(""),0,1,0,0,1,ParameterInfo::kCanAutomate|ParameterInfo::kIsBypass);
 return kResultOk;
}
tresult PLUGIN_API TapeDriveController::setComponentState(IBStream*s){
 if(!s)return kResultFalse; IBStreamer b(s,kLittleEndian); float v[6]{};
 for(auto&x:v) if(!b.readFloat(x)) return kResultFalse;
 setParamNormalized(kSatId,v[0]); setParamNormalized(kLevelId,v[1]); setParamNormalized(kDryId,v[2]);
 setParamNormalized(kGainModeId,v[3]); setParamNormalized(kShiftId,v[4]); setParamNormalized(kDriveBypassId,v[5]);
 return kResultOk;
}
}
