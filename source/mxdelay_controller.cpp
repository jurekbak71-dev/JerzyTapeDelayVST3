#include "mxdelay_controller.h"
#include "mxdelay_params.h"
#include "mxdelay_state.h"
#include "tapedrive_gui_views.h"
#include "base/source/fstreamer.h"
#include "base/source/fstring.h"
#include "public.sdk/source/vst/vstparameters.h"
#include <cstring>
#include <string>
using namespace Steinberg;using namespace Steinberg::Vst;
namespace JerzyAudio {
namespace {
StringListParameter* listParam(const TChar* name,ParamID id,std::initializer_list<const TChar*> items,int def=0,int32 flags=ParameterInfo::kCanAutomate){auto*p=new StringListParameter(name,id,nullptr,flags);for(auto*x:items)p->appendString(x);if(items.size()>1)p->setNormalized(double(def)/double(items.size()-1));return p;}
RangeParameter* rangeParam(const TChar* name,ParamID id,const TChar* unit,double lo,double hi,double def,int prec=1,int32 steps=0,int32 flags=ParameterInfo::kCanAutomate){auto*p=new RangeParameter(name,id,unit,lo,hi,def,steps,flags);p->setPrecision(prec);return p;}
void addPercent(ParameterContainer& ps,const TChar* name,ParamID id,double def){ps.addParameter(rangeParam(name,id,STR16("%"),0,100,def*100,1));}
}
tresult PLUGIN_API MXDelayController::initialize(FUnknown*c){
 registerTapeDriveViews();auto r=EditControllerEx1::initialize(c);if(r!=kResultOk)return r;MXDelayParams d;
 addPercent(parameters,STR16("Mix"),kMXMixId,d.mix);parameters.addParameter(rangeParam(STR16("Input Trim"),kMXInputTrimId,STR16("dB"),-18,18,0,1));parameters.addParameter(rangeParam(STR16("Output Trim"),kMXOutputTrimId,STR16("dB"),-18,12,-3,1));
 parameters.addParameter(listParam(STR16("Routing"),kMXRoutingId,{STR16("SERIES"),STR16("PARALLEL"),STR16("SPLIT L/R")},0));
 parameters.addParameter(rangeParam(STR16("Bypass"),kMXBypassId,STR16(""),0,1,0,0,1,ParameterInfo::kCanAutomate|ParameterInfo::kIsBypass));
 parameters.addParameter(rangeParam(STR16("Spillover"),kMXSpillId,STR16(""),0,1,1,0,1));parameters.addParameter(rangeParam(STR16("Host Tempo"),kMXTempoMeterId,STR16("BPM"),0,300,120,1,0,ParameterInfo::kIsReadOnly));
 const char* algNames[]={"MultiHead Reel","Tape Echo","Oil Can","Tube Echo","BBD","Doubletrack","Dual Digital"};
 const char* ctl[kAlgorithmCount][kAlgoControls]={
 {"Mechanics","Wear","Spacing","Low Cut","Head Spread","Saturation","Play Head 1","Play Head 2","Play Head 3","Play Head 4","Feedback Head 1","Feedback Head 2","Feedback Head 3","Feedback Head 4","Unused 15","Unused 16"},
 {"Tape Age","Wow","Flutter","Crinkle","Bias","Low Contour","Spring","Machine Mode","Unused 9","Unused 10","Unused 11","Unused 12","Unused 13","Unused 14","Unused 15","Unused 16"},
 {"Disc Wear","Viscosity","Static","Head Mix","Tone","Drift","Unused 7","Unused 8","Unused 9","Unused 10","Unused 11","Unused 12","Unused 13","Unused 14","Unused 15","Unused 16"},
 {"Mechanics","Tape Age","Bias","Tube Preamp","Record Level","Stereo Spread","Unused 7","Unused 8","Unused 9","Unused 10","Unused 11","Unused 12","Unused 13","Unused 14","Unused 15","Unused 16"},
 {"Voice","Filter","Mod Depth","Mod Rate","Companding","BBD Noise","Unused 7","Unused 8","Unused 9","Unused 10","Unused 11","Unused 12","Unused 13","Unused 14","Unused 15","Unused 16"},
 {"Saturation","Wobble","Blend","Blend Type","Stereo Width","Auto-Flange","Unused 7","Unused 8","Unused 9","Unused 10","Unused 11","Unused 12","Unused 13","Unused 14","Unused 15","Unused 16"},
 {"Converter","Delay 2 Ratio","Mod Depth","Cross Feedback","Repeat Dynamics","Tone","Unused 7","Unused 8","Unused 9","Unused 10","Unused 11","Unused 12","Unused 13","Unused 14","Unused 15","Unused 16"}};
 for(int s=0;s<2;++s){
  const auto prefix=s==0?"A · ":"B · ";
  auto* ap=new StringListParameter(s==0?STR16("A · Algorithm"):STR16("B · Algorithm"),slotParam(s,kSlotAlgorithm));for(auto*n:algNames){String128 t{};UString(t,128).fromAscii(n);ap->appendString(t);}ap->setNormalized(d.slot[s].algorithm);parameters.addParameter(ap);
  parameters.addParameter(rangeParam(s==0?STR16("A · Enable"):STR16("B · Enable"),slotParam(s,kSlotEnable),STR16(""),0,1,1,0,1));parameters.addParameter(rangeParam(s==0?STR16("A · Sync"):STR16("B · Sync"),slotParam(s,kSlotSync),STR16(""),0,1,1,0,1));
  parameters.addParameter(listParam(s==0?STR16("A · Division"):STR16("B · Division"),slotParam(s,kSlotDivision),{STR16("1/1"),STR16("1/2."),STR16("1/2"),STR16("1/4."),STR16("1/4"),STR16("1/8."),STR16("1/8"),STR16("1/4T"),STR16("1/16"),STR16("1/8T")},4));
  parameters.addParameter(rangeParam(s==0?STR16("A · Free Time"):STR16("B · Free Time"),slotParam(s,kSlotTime),STR16("ms"),1,2500,500,1));addPercent(parameters,s==0?STR16("A · Feedback"):STR16("B · Feedback"),slotParam(s,kSlotFeedback),d.slot[s].feedback);addPercent(parameters,s==0?STR16("A · Level"):STR16("B · Level"),slotParam(s,kSlotLevel),d.slot[s].level);parameters.addParameter(rangeParam(s==0?STR16("A · Pan"):STR16("B · Pan"),slotParam(s,kSlotPan),STR16("%"),-100,100,0,1));addPercent(parameters,s==0?STR16("A · Duck"):STR16("B · Duck"),slotParam(s,kSlotDuck),d.slot[s].duck);
  for(int a=0;a<kAlgorithmCount;++a)for(int k=0;k<kAlgoControls;++k){if(std::strncmp(ctl[a][k],"Unused",6)==0)continue;std::string n=prefix+std::string(algNames[a])+" · "+ctl[a][k];String128 tn{};UString(tn,128).fromAscii(n.c_str());auto id=algoParam(s,a,k);double def=d.slot[s].c[a][k];
   if(a==(int)DelayAlgorithm::Brig&&k==0)parameters.addParameter(listParam(tn,id,{STR16("3205"),STR16("3005"),STR16("MULTI")},1));
   else if(a==(int)DelayAlgorithm::ElCapistan&&k==7)parameters.addParameter(listParam(tn,id,{STR16("FIXED"),STR16("MULTI"),STR16("SINGLE")},1));
   else if(a==(int)DelayAlgorithm::Deco&&k==3)parameters.addParameter(listParam(tn,id,{STR16("SUM"),STR16("INVERT"),STR16("BOUNCE")},0));
   else if(a==(int)DelayAlgorithm::DIG&&k==0)parameters.addParameter(listParam(tn,id,{STR16("24/96"),STR16("ADM"),STR16("12 BIT")},0));
   else if(a==(int)DelayAlgorithm::Volante&&(k>=6&&k<=13))parameters.addParameter(rangeParam(tn,id,STR16(""),0,1,def>=.5?1:0,0,1));
   else addPercent(parameters,tn,id,def);
  }
 }
 return kResultOk;
}
tresult PLUGIN_API MXDelayController::setComponentState(IBStream*s){if(!s)return kResultFalse;IBStreamer b(s,kLittleEndian);MXDelayParams p;if(!readMXState(b,p))return kResultFalse;setParamNormalized(kMXMixId,p.mix);setParamNormalized(kMXInputTrimId,p.inputTrim);setParamNormalized(kMXOutputTrimId,p.outputTrim);setParamNormalized(kMXRoutingId,p.routing);setParamNormalized(kMXBypassId,p.bypass);setParamNormalized(kMXSpillId,p.spill);for(int x=0;x<2;++x){auto&q=p.slot[x];setParamNormalized(slotParam(x,kSlotAlgorithm),q.algorithm);setParamNormalized(slotParam(x,kSlotEnable),q.enable);setParamNormalized(slotParam(x,kSlotSync),q.sync);setParamNormalized(slotParam(x,kSlotDivision),q.division);setParamNormalized(slotParam(x,kSlotTime),q.time);setParamNormalized(slotParam(x,kSlotFeedback),q.feedback);setParamNormalized(slotParam(x,kSlotLevel),q.level);setParamNormalized(slotParam(x,kSlotPan),q.pan);setParamNormalized(slotParam(x,kSlotDuck),q.duck);for(int a=0;a<kAlgorithmCount;++a)for(int k=0;k<kAlgoControls;++k)if(parameters.getParameter(algoParam(x,a,k)))setParamNormalized(algoParam(x,a,k),q.c[a][k]);}return kResultOk;}
IPlugView* PLUGIN_API MXDelayController::createView(const char*n){if(n&&std::strcmp(n,ViewType::kEditor)==0){auto*e=new VSTGUI::VST3Editor(this,"view","mxdelay.uidesc");e->setAllowedZoomFactors({0.75,1.0,1.25,1.5});return e;}return nullptr;}
tresult PLUGIN_API MXDelayController::getMidiControllerAssignment(int32 bus,int16,CtrlNumber cc,ParamID&id){if(bus!=0)return kResultFalse;switch((int)cc){case 1:id=slotParam(0,kSlotFeedback);break;case 7:id=kMXMixId;break;case 11:id=kMXInputTrimId;break;case 64:id=kMXBypassId;break;case 71:id=slotParam(0,kSlotFeedback);break;case 72:id=slotParam(1,kSlotFeedback);break;case 73:id=slotParam(1,kSlotTime);break;case 74:id=slotParam(0,kSlotTime);break;case 76:id=slotParam(0,kSlotPan);break;case 77:id=slotParam(1,kSlotPan);break;case 91:id=slotParam(0,kSlotLevel);break;case 93:id=slotParam(1,kSlotLevel);break;default:return kResultFalse;}return kResultTrue;}
tresult PLUGIN_API MXDelayController::queryInterface(const char*iid,void**obj){QUERY_INTERFACE(iid,obj,IMidiMapping::iid,IMidiMapping)return EditControllerEx1::queryInterface(iid,obj);}
}
