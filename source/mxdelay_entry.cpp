#include "mxdelay_controller.h"
#include "mxdelay_ids.h"
#include "mxdelay_processor.h"
#include "mxdelay_version.h"
#include "public.sdk/source/main/pluginfactory_constexpr.h"
using namespace Steinberg;using namespace Steinberg::Vst;
BEGIN_FACTORY_DEF(stringCompanyName,stringCompanyWeb,stringCompanyEmail,2)
DEF_CLASS(JerzyAudio::kMXDelayProcessorUID,PClassInfo::kManyInstances,kVstAudioEffectClass,"Jerzy MX Analog Delay",Vst::kDistributable,"Fx|Delay|Stereo",FULL_VERSION_STR,kVstVersionString,JerzyAudio::MXDelayProcessor::createInstance,nullptr)
DEF_CLASS(JerzyAudio::kMXDelayControllerUID,PClassInfo::kManyInstances,kVstComponentControllerClass,"Jerzy MX Analog Delay Controller",0,"",FULL_VERSION_STR,kVstVersionString,JerzyAudio::MXDelayController::createInstance,nullptr)
END_FACTORY
