#pragma once
#include "mxdelay_params.h"
#include <algorithm>
#include <cmath>

namespace JerzyAudio {
constexpr int kMXStateVersion=2;

template<class Stream> bool writeMXState(Stream& b,const MXDelayParams& p){
    if(!b.writeInt32(kMXStateVersion)) return false;
    const double global[]={p.mix,p.inputTrim,p.outputTrim,p.routing,p.bypass,p.spill};
    for(double v:global) if(!b.writeFloat((float)v)) return false;
    for(int s=0;s<2;++s){
        const auto& x=p.slot[s];
        const double common[]={x.algorithm,x.enable,x.sync,x.division,x.time,x.feedback,x.level,x.pan,x.duck};
        for(double v:common) if(!b.writeFloat((float)v)) return false;
        for(int a=0;a<kAlgorithmCount;++a) for(int c=0;c<kAlgoControls;++c)
            if(!b.writeFloat((float)x.c[a][c])) return false;
        for(double v:x.headPan) if(!b.writeFloat((float)v)) return false;
    }
    return true;
}

template<class Stream> bool readMXState(Stream& b,MXDelayParams& p){
    p=MXDelayParams{};
    int32_t version=0; if(!b.readInt32(version)) return false;
    if(version<1 || version>kMXStateVersion) return false;
    auto rd=[&](double& v){ float x=0; if(!b.readFloat(x)||!std::isfinite(x)) return false; v=std::clamp((double)x,0.0,1.0); return true; };
    double* global[]={&p.mix,&p.inputTrim,&p.outputTrim,&p.routing,&p.bypass,&p.spill};
    for(auto* v:global) if(!rd(*v)) return false;
    for(int s=0;s<2;++s){
        auto& x=p.slot[s];
        double* common[]={&x.algorithm,&x.enable,&x.sync,&x.division,&x.time,&x.feedback,&x.level,&x.pan,&x.duck};
        for(auto* v:common) if(!rd(*v)) return false;
        for(int a=0;a<kAlgorithmCount;++a) for(int c=0;c<kAlgoControls;++c) if(!rd(x.c[a][c])) return false;
        if(version>=2) for(auto& v:x.headPan) if(!rd(v)) return false;
    }
    return true;
}
}
