#include "mxdelay_processor.h"
#include "mxdelay_ids.h"
#include "mxdelay_state.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <algorithm>
#include <cmath>
using namespace Steinberg;using namespace Steinberg::Vst;
namespace JerzyAudio {
MXDelayProcessor::MXDelayProcessor(){setControllerClass(kMXDelayControllerUID);processContextRequirements.needTempo().needTransportState();}
tresult PLUGIN_API MXDelayProcessor::initialize(FUnknown* c){auto r=AudioEffect::initialize(c);if(r!=kResultOk)return r;addAudioInput(STR16("Stereo In"),SpeakerArr::kStereo);addAudioOutput(STR16("Stereo Out"),SpeakerArr::kStereo);addEventInput(STR16("MIDI In"),16);return kResultOk;}
tresult PLUGIN_API MXDelayProcessor::setBusArrangements(SpeakerArrangement* i,int32 ni,SpeakerArrangement* o,int32 no){if(ni!=1||no!=1)return kResultFalse;auto a=SpeakerArr::getChannelCount(i[0]),b=SpeakerArr::getChannelCount(o[0]);if(a!=b||(a!=1&&a!=2))return kResultFalse;return AudioEffect::setBusArrangements(i,ni,o,no);}
tresult PLUGIN_API MXDelayProcessor::setupProcessing(ProcessSetup& s){sampleRate=s.sampleRate>0?s.sampleRate:44100;dsp32.prepare(sampleRate,2);dsp64.prepare(sampleRate,2);return AudioEffect::setupProcessing(s);}
tresult PLUGIN_API MXDelayProcessor::setActive(TBool x){if(x){dsp32.reset();dsp64.reset();lastPeak=0;}return AudioEffect::setActive(x);}
tresult PLUGIN_API MXDelayProcessor::canProcessSampleSize(int32 s){return(s==kSample32||s==kSample64)?kResultTrue:kResultFalse;}
void MXDelayProcessor::setNormalized(ParamID id,double v){v=std::clamp(v,0.0,1.0);switch(id){case kMXMixId:p.mix=v;return;case kMXInputTrimId:p.inputTrim=v;return;case kMXOutputTrimId:p.outputTrim=v;return;case kMXRoutingId:p.routing=v;return;case kMXBypassId:p.bypass=v;return;case kMXSpillId:p.spill=v;return;default:break;}for(int s=0;s<2;++s){unsigned base=slotBase(s);if(id<base||id>=base+kMXSlotStride)continue;unsigned off=id-base;auto& x=p.slot[s];if(off<=kSlotDuck){switch(off){case kSlotAlgorithm:x.algorithm=v;break;case kSlotEnable:x.enable=v;break;case kSlotSync:x.sync=v;break;case kSlotDivision:x.division=v;break;case kSlotTime:x.time=v;break;case kSlotFeedback:x.feedback=v;break;case kSlotLevel:x.level=v;break;case kSlotPan:x.pan=v;break;case kSlotDuck:x.duck=v;break;default:break;}return;}if(off>=kSlotHeadPan1&&off<=kSlotHeadPan4){x.headPan[off-kSlotHeadPan1]=v;return;}if(off>=kSlotControlBase){unsigned q=off-kSlotControlBase;int a=(int)(q/kAlgoControls),c=(int)(q%kAlgoControls);if(a<kAlgorithmCount){x.c[a][c]=v;return;}}}}
void MXDelayProcessor::readChanges(IParameterChanges* changes){if(!changes)return;for(int32 i=0;i<changes->getParameterCount();++i)if(auto*q=changes->getParameterData(i)){int32 n=q->getPointCount();if(n<=0)continue;int32 off=0;ParamValue v=0;if(q->getPoint(n-1,off,v)==kResultTrue&&std::isfinite(v))setNormalized(q->getParameterId(),v);}}
void MXDelayProcessor::sendTempo(ProcessData& d){if(!d.outputParameterChanges)return;int32 qi=0;if(auto*q=d.outputParameterChanges->addParameterData(kMXTempoMeterId,qi)){int32 pi=0;q->addPoint(std::max<int32>(0,d.numSamples-1),std::clamp(bpm/300.0,0.0,1.0),pi);}}
tresult PLUGIN_API MXDelayProcessor::process(ProcessData& d){readChanges(d.inputParameterChanges);if(d.processContext&&(d.processContext->state&ProcessContext::kTempoValid)&&std::isfinite(d.processContext->tempo)&&d.processContext->tempo>1.0)bpm=d.processContext->tempo;if(d.numInputs==0||d.numOutputs==0||d.numSamples<=0){sendTempo(d);return kResultOk;}int ch=std::min(d.inputs[0].numChannels,d.outputs[0].numChannels);if(ch<=0)return kResultOk;if(d.symbolicSampleSize==kSample32)dsp32.process(d.inputs[0].channelBuffers32,d.outputs[0].channelBuffers32,ch,d.numSamples,p,bpm,lastPeak);else if(d.symbolicSampleSize==kSample64)dsp64.process(d.inputs[0].channelBuffers64,d.outputs[0].channelBuffers64,ch,d.numSamples,p,bpm,lastPeak);d.outputs[0].silenceFlags=0;sendTempo(d);return kResultOk;}
tresult PLUGIN_API MXDelayProcessor::setState(IBStream*s){if(!s)return kResultFalse;IBStreamer b(s,kLittleEndian);return readMXState(b,p)?kResultOk:kResultFalse;}
tresult PLUGIN_API MXDelayProcessor::getState(IBStream*s){if(!s)return kResultFalse;IBStreamer b(s,kLittleEndian);return writeMXState(b,p)?kResultOk:kResultFalse;}
}
