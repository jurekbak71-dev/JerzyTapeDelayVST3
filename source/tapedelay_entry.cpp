#include "tapedelay_controller.h"
#include "tapedelay_ids.h"
#include "tapedelay_processor.h"
#include "version.h"
#include "public.sdk/source/main/pluginfactory_constexpr.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF(stringCompanyName, stringCompanyWeb, stringCompanyEmail, 2)

DEF_CLASS(JerzyAudio::kProcessorUID,
          PClassInfo::kManyInstances,
          kVstAudioEffectClass,
          stringPluginName,
          Vst::kDistributable,
          "Fx|Delay",
          FULL_VERSION_STR,
          kVstVersionString,
          JerzyAudio::TapeDelayProcessor::createInstance,
          nullptr)

DEF_CLASS(JerzyAudio::kControllerUID,
          PClassInfo::kManyInstances,
          kVstComponentControllerClass,
          stringPluginName " Controller",
          0,
          "",
          FULL_VERSION_STR,
          kVstVersionString,
          JerzyAudio::TapeDelayController::createInstance,
          nullptr)

END_FACTORY
