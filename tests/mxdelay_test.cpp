#include "mxdelay_dsp.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace JerzyAudio;
int main(){
    constexpr int N=48000; std::vector<float> inL(N),inR(N),outL(N),outR(N); inL[0]=inR[0]=1.0f;
    float* in[2]={inL.data(),inR.data()};float* out[2]={outL.data(),outR.data()};
    MXDelayDSP<float> dsp;dsp.prepare(48000,2);MXDelayParams p;p.mix=1.0;p.outputTrim=0.6;p.inputTrim=0.5;
    double peak=0;
    for(int algo=0;algo<kAlgorithmCount;++algo){
        dsp.reset();std::fill(outL.begin(),outL.end(),0);std::fill(outR.begin(),outR.end(),0);p.slot[0].algorithm=algo/6.0;p.slot[1].enable=0.0;p.slot[0].sync=0.0;p.slot[0].time=0.06;p.slot[0].feedback=0.45;
        dsp.process(in,out,2,N,p,120.0,peak);assert(std::isfinite(peak));assert(peak<5.0);
        double energy=0;for(int i=1;i<N;++i)energy+=std::abs(outL[i])+std::abs(outR[i]);assert(energy>1e-4);
        std::cout<<"algo "<<algo<<" peak="<<peak<<" energy="<<energy<<"\n";
    }
    SlotParams s;s.sync=1.0;s.division=4.0/9.0;assert(std::abs(slotDelayMs(s,120.0)-500.0)<1e-6);
    return 0;
}
