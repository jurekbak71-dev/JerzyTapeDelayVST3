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
 return AudioEffect::setupProcessing(s);
}

tresult PLUGIN_API TapeDriveProcessor::setActive(TBool s){
 if(s){dsp32.reset();dsp64.reset();}
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
   default:break;
  }
 }
}

tresult PLUGIN_API TapeDriveProcessor::process(ProcessData& d){
 readChanges(d.inputParameterChanges);
 if(d.numInputs==0||d.numOutputs==0||d.numSamples<=0)return kResultOk;
 int ch=std::min(d.inputs[0].numChannels,d.outputs[0].numChannels); if(ch<=0)return kResultOk;

 const bool bp=p.bypass>=0.5;
 const int gm=p.gainMode>=0.5?1:0;
 const int sm=std::clamp((int)std::lround(p.shift*2.0),0,2);

 if(d.symbolicSampleSize==kSample32){
  auto**in=d.inputs[0].channelBuffers32; auto**out=d.outputs[0].channelBuffers32;
  if(bp){
   for(int c=0;c<ch;++c) if(in[c]&&out[c]&&in[c]!=out[c])
    std::memcpy(out[c],in[c],sizeof(float)*(size_t)d.numSamples);
  } else {
   dsp32.process(in,out,ch,d.numSamples,p.sat,p.level,p.dry,gm,sm,p.hpfCutoff,p.hpfRes,p.lpfCutoff,p.lpfRes);
  }
 } else if(d.symbolicSampleSize==kSample64){
  auto**in=d.inputs[0].channelBuffers64; auto**out=d.outputs[0].channelBuffers64;
  if(bp){
   for(int c=0;c<ch;++c) if(in[c]&&out[c]&&in[c]!=out[c])
    std::memcpy(out[c],in[c],sizeof(double)*(size_t)d.numSamples);
  } else {
   dsp64.process(in,out,ch,d.numSamples,p.sat,p.level,p.dry,gm,sm,p.hpfCutoff,p.hpfRes,p.lpfCutoff,p.lpfRes);
  }
 }
 d.outputs[0].silenceFlags=0;
 return kResultOk;
}

tresult PLUGIN_API TapeDriveProcessor::setState(IBStream*s){
 if(!s)return kResultFalse;
 IBStreamer b(s,kLittleEndian);
 float v[10]{};
 for(int i=0;i<6;++i) if(!b.readFloat(v[i])) return kResultFalse;
 p.sat=v[0]; p.level=v[1]; p.dry=v[2]; p.gainMode=v[3]; p.shift=v[4]; p.bypass=v[5];
 if(b.readFloat(v[6]))p.hpfCutoff=v[6];
 if(b.readFloat(v[7]))p.hpfRes=v[7];
 if(b.readFloat(v[8]))p.lpfCutoff=v[8];
 if(b.readFloat(v[9]))p.lpfRes=v[9];
 return kResultOk;
}

tresult PLUGIN_API TapeDriveProcessor::getState(IBStream*s){
 if(!s)return kResultFalse;
 IBStreamer b(s,kLittleEndian);
 float v[10]={
  (float)p.sat,(float)p.level,(float)p.dry,(float)p.gainMode,(float)p.shift,(float)p.bypass,
  (float)p.hpfCutoff,(float)p.hpfRes,(float)p.lpfCutoff,(float)p.lpfRes
 };
 for(auto x:v) if(!b.writeFloat(x)) return kResultFalse;
 return kResultOk;
}

}
