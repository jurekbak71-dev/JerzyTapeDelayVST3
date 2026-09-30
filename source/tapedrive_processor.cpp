#include "tapedrive_processor.h"
#include "tapedrive_ids.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Steinberg; using namespace Steinberg::Vst;
namespace JerzyAudio {

TapeDriveProcessor::TapeDriveProcessor(){ setControllerClass(kTapeDriveControllerUID); }

tresult PLUGIN_API TapeDriveProcessor::initialize(FUnknown* c){
 auto r=AudioEffect::initialize(c); if(r!=kResultOk)return r;
 addAudioInput(STR16("Stereo In"),SpeakerArr::kStereo);
 addAudioOutput(STR16("Stereo Out"),SpeakerArr::kStereo);
 return kResultOk;
}

tresult PLUGIN_API TapeDriveProcessor::setBusArrangements(SpeakerArrangement* i,int32 ni,SpeakerArrangement* o,int32 no){
 if(ni!=1||no!=1)return kResultFalse;
 auto a=SpeakerArr::getChannelCount(i[0]); auto b=SpeakerArr::getChannelCount(o[0]);
 if(a!=b||(a!=1&&a!=2))return kResultFalse;
 return AudioEffect::setBusArrangements(i,ni,o,no);
}

tresult PLUGIN_API TapeDriveProcessor::setupProcessing(ProcessSetup& s){
 sr=s.sampleRate>0?s.sampleRate:44100;
 dsp32.prepare(sr,2); dsp64.prepare(sr,2);
 inMeter=satMeter=outMeter=0.0;
 return AudioEffect::setupProcessing(s);
}

tresult PLUGIN_API TapeDriveProcessor::setActive(TBool s){
 if(s){
  dsp32.reset(); dsp64.reset();
  inMeter=satMeter=outMeter=0.0;
 }
 return AudioEffect::setActive(s);
}

tresult PLUGIN_API TapeDriveProcessor::canProcessSampleSize(int32 s){
 return(s==kSample32||s==kSample64)?kResultTrue:kResultFalse;
}

void TapeDriveProcessor::readChanges(IParameterChanges* c){
 if(!c)return;
 for(int32 i=0;i<c->getParameterCount();++i) if(auto*q=c->getParameterData(i)){
  int32 n=q->getPointCount(); if(n<=0)continue;
  int32 off=0; ParamValue v=0;
  if(q->getPoint(n-1,off,v)!=kResultTrue)continue;
  v=std::clamp(v,0.0,1.0);
  switch(q->getParameterId()){
   case kSatId:p.sat=v;break;
   case kLevelId:p.level=v;break;
   case kDryId:p.dry=v;break;
   case kGainModeId:p.gainMode=v;break;
   case kShiftId:p.shift=v;break;
   case kDriveBypassId:p.bypass=v;break;
   case kHPFCutoffId:p.hpfCutoff=v;break;
   case kHPFResId:p.hpfRes=v;break;
   case kLPFCutoffId:p.lpfCutoff=v;break;
   case kLPFResId:p.lpfRes=v;break;
   case kWowFlutterId:p.wowFlutter=v;break;
   default:break;
  }
 }
}

void TapeDriveProcessor::sendMeters(ProcessData& d,double inPk,double satPk,double outPk){
 const auto smooth=[](double value,double oldValue){
  value=std::clamp(value,0.0,1.0);
  return std::max(value,oldValue*0.88);
 };
 inMeter=smooth(std::pow(std::clamp(inPk,0.0,1.0),0.35),inMeter);
 satMeter=smooth(std::clamp(satPk,0.0,1.0),satMeter);
 outMeter=smooth(std::pow(std::clamp(outPk,0.0,1.0),0.35),outMeter);

 if(!d.outputParameterChanges)return;
 struct M { ParamID id; double value; } meters[]={
  {kInputMeterId,inMeter},{kSaturationMeterId,satMeter},{kDriveMeterId,outMeter}
 };
 for(const auto& m:meters){
  int32 queueIndex=0;
  if(auto*q=d.outputParameterChanges->addParameterData(m.id,queueIndex)){
   int32 pointIndex=0;
   q->addPoint(std::max<int32>(0,d.numSamples-1),m.value,pointIndex);
  }
 }
}

tresult PLUGIN_API TapeDriveProcessor::process(ProcessData& d){
 readChanges(d.inputParameterChanges);
 if(d.numInputs==0||d.numOutputs==0||d.numSamples<=0){
  sendMeters(d,0,0,0);
  return kResultOk;
 }

 int ch=std::min(d.inputs[0].numChannels,d.outputs[0].numChannels); if(ch<=0)return kResultOk;
 const bool bp=p.bypass>=0.5;
 const int gm=p.gainMode>=0.5?1:0;
 const int sm=std::clamp((int)std::lround(p.shift*2.0),0,2);
 double inPk=0.0,satPk=0.0,outPk=0.0;

 if(d.symbolicSampleSize==kSample32){
  auto**in=d.inputs[0].channelBuffers32; auto**out=d.outputs[0].channelBuffers32;
  if(bp){
   for(int c=0;c<ch;++c){
    if(in[c]&&out[c]&&in[c]!=out[c]) std::memcpy(out[c],in[c],sizeof(float)*(size_t)d.numSamples);
    if(in[c]) for(int i=0;i<d.numSamples;++i) inPk=std::max(inPk,std::abs((double)in[c][i]));
    if(out[c]) for(int i=0;i<d.numSamples;++i) outPk=std::max(outPk,std::abs((double)out[c][i]));
   }
  }else{
   dsp32.process(in,out,ch,d.numSamples,p.sat,p.level,p.dry,gm,sm,p.hpfCutoff,p.hpfRes,p.lpfCutoff,p.lpfRes,p.wowFlutter,inPk,satPk,outPk);
  }
 }else if(d.symbolicSampleSize==kSample64){
  auto**in=d.inputs[0].channelBuffers64; auto**out=d.outputs[0].channelBuffers64;
  if(bp){
   for(int c=0;c<ch;++c){
    if(in[c]&&out[c]&&in[c]!=out[c]) std::memcpy(out[c],in[c],sizeof(double)*(size_t)d.numSamples);
    if(in[c]) for(int i=0;i<d.numSamples;++i) inPk=std::max(inPk,std::abs(in[c][i]));
    if(out[c]) for(int i=0;i<d.numSamples;++i) outPk=std::max(outPk,std::abs(out[c][i]));
   }
  }else{
   dsp64.process(in,out,ch,d.numSamples,p.sat,p.level,p.dry,gm,sm,p.hpfCutoff,p.hpfRes,p.lpfCutoff,p.lpfRes,p.wowFlutter,inPk,satPk,outPk);
  }
 }

 d.outputs[0].silenceFlags=0;
 sendMeters(d,inPk,satPk,outPk);
 return kResultOk;
}

tresult PLUGIN_API TapeDriveProcessor::setState(IBStream*s){
 if(!s)return kResultFalse;
 IBStreamer b(s,kLittleEndian);
 float v[11]{};
 for(int i=0;i<6;++i) if(!b.readFloat(v[i])) return kResultFalse;
 p.sat=v[0]; p.level=v[1]; p.dry=v[2]; p.gainMode=v[3]; p.shift=v[4]; p.bypass=v[5];
 if(b.readFloat(v[6]))p.hpfCutoff=v[6];
 if(b.readFloat(v[7]))p.hpfRes=v[7];
 if(b.readFloat(v[8]))p.lpfCutoff=v[8];
 if(b.readFloat(v[9]))p.lpfRes=v[9];
 if(b.readFloat(v[10]))p.wowFlutter=v[10];
 return kResultOk;
}

tresult PLUGIN_API TapeDriveProcessor::getState(IBStream*s){
 if(!s)return kResultFalse;
 IBStreamer b(s,kLittleEndian);
 float v[11]={
  (float)p.sat,(float)p.level,(float)p.dry,(float)p.gainMode,(float)p.shift,(float)p.bypass,
  (float)p.hpfCutoff,(float)p.hpfRes,(float)p.lpfCutoff,(float)p.lpfRes,(float)p.wowFlutter
 };
 for(auto x:v) if(!b.writeFloat(x)) return kResultFalse;
 return kResultOk;
}

}
