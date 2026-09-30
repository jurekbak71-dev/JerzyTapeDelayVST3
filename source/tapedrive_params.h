#pragma once
namespace JerzyAudio {
enum TapeDriveParamIds : unsigned int {
    kSatId = 300,
    kLevelId,
    kDryId,
    kGainModeId,
    kShiftId,
    kDriveBypassId,
    kDriveMeterId
};
struct TapeDriveParams {
    double sat=0.35;
    double level=0.5;
    double dry=0.0;
    double gainMode=0.0;
    double shift=0.5;
    double bypass=0.0;
};
}
