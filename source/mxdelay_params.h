#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace JerzyAudio {

enum class DelayAlgorithm : int { Volante=0, ElCapistan, Olivera, EC1, Brig, Deco, DIG, Count };
enum class DelayRouting : int { Series=0, Parallel, SplitLR };
constexpr int kAlgorithmCount = static_cast<int>(DelayAlgorithm::Count);
constexpr int kAlgoControls = 16;
constexpr unsigned kMXGlobalBase = 1000;
constexpr unsigned kMXSlotABase = 1100;
constexpr unsigned kMXSlotBBase = 1400;
constexpr unsigned kMXSlotStride = 300;

enum MXGlobalParamIds : unsigned {
    kMXMixId = kMXGlobalBase,
    kMXInputTrimId,
    kMXOutputTrimId,
    kMXRoutingId,
    kMXBypassId,
    kMXSpillId,
    kMXTempoMeterId
};

enum SlotOffsets : unsigned {
    kSlotAlgorithm = 0,
    kSlotEnable = 1,
    kSlotSync = 2,
    kSlotDivision = 3,
    kSlotTime = 4,
    kSlotFeedback = 5,
    kSlotLevel = 6,
    kSlotPan = 7,
    kSlotDuck = 8,
    kSlotControlBase = 20
};

constexpr unsigned slotBase(int slot){ return slot==0 ? kMXSlotABase : kMXSlotBBase; }
constexpr unsigned slotParam(int slot,unsigned offset){ return slotBase(slot)+offset; }
constexpr unsigned algoParam(int slot,int algo,int control){ return slotBase(slot)+kSlotControlBase+unsigned(algo*kAlgoControls+control); }

struct SlotParams {
    double algorithm=0.0;
    double enable=1.0;
    double sync=1.0;
    double division=0.50;
    double time=0.20;
    double feedback=0.42;
    double level=0.78;
    double pan=0.50;
    double duck=0.0;
    std::array<std::array<double,kAlgoControls>,kAlgorithmCount> c{};
};

struct MXDelayParams {
    double mix=0.35;
    double inputTrim=0.50;
    double outputTrim=0.50;
    double routing=0.0;
    double bypass=0.0;
    double spill=1.0;
    SlotParams slot[2];
    MXDelayParams(){
        slot[0].algorithm=0.0;
        slot[1].algorithm=1.0;
        for(auto& s:slot){
            s.c[(int)DelayAlgorithm::Volante][0]=0.24;
            s.c[(int)DelayAlgorithm::Volante][1]=0.20;
            s.c[(int)DelayAlgorithm::Volante][2]=0.10;
            s.c[(int)DelayAlgorithm::Volante][3]=0.15;
            s.c[(int)DelayAlgorithm::Volante][4]=0.65;
            s.c[(int)DelayAlgorithm::Volante][5]=0.26;
            for(int i=6;i<10;++i) s.c[(int)DelayAlgorithm::Volante][i]=1.0;
            for(int i=10;i<14;++i) s.c[(int)DelayAlgorithm::Volante][i]=(i==13)?1.0:0.0;
            s.c[(int)DelayAlgorithm::ElCapistan] = {0.32,0.18,0.12,0.08,0.50,0.35,0.12,0.33};
            s.c[(int)DelayAlgorithm::Olivera] = {0.38,0.55,0.22,0.50,0.35,0.28,0.0,0.0};
            s.c[(int)DelayAlgorithm::EC1] = {0.18,0.25,0.50,0.42,0.35,0.60,0.0,0.0};
            s.c[(int)DelayAlgorithm::Brig] = {0.50,0.55,0.18,0.30,0.55,0.10,0.0,0.0};
            s.c[(int)DelayAlgorithm::Deco] = {0.30,0.15,0.58,0.0,0.75,0.0,0.0,0.0};
            s.c[(int)DelayAlgorithm::DIG] = {0.0,0.50,0.12,0.25,0.30,0.55,0.0,0.0};
        }
    }
};

inline int normIndex(double n,int count){
    if(count<=1) return 0;
    return std::clamp((int)std::lround(std::clamp(n,0.0,1.0)*(count-1)),0,count-1);
}
inline double dbFromNorm(double n,double lo=-18.0,double hi=12.0){ return lo+(hi-lo)*std::clamp(n,0.0,1.0); }
inline double gainFromNorm(double n,double lo=-18.0,double hi=12.0){ return std::pow(10.0,dbFromNorm(n,lo,hi)/20.0); }
inline double freeTimeMs(double n){ return 1.0 + std::clamp(n,0.0,1.0)*2499.0; }
inline double divisionBeats(int idx){
    static constexpr double beats[]={4.0,3.0,2.0,1.5,1.0,0.75,0.5,1.0/3.0,0.25,1.0/6.0};
    return beats[std::clamp(idx,0,9)];
}
inline double slotDelayMs(const SlotParams& s,double bpm){
    if(s.sync>=0.5 && bpm>1.0){
        const int div=normIndex(s.division,10);
        return 60000.0/bpm*divisionBeats(div);
    }
    return freeTimeMs(s.time);
}

} // namespace JerzyAudio
