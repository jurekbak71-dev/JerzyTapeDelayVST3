#pragma once
#include "mxdelay_params.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace JerzyAudio {

template<class T> class MXDelayDSP {
    static constexpr double pi=3.14159265358979323846;
    struct Stereo { T l{},r{}; };
    struct SlotDSP {
        double sr=44100.0;
        std::vector<T> l,r; size_t w=0;
        T lpL{},lpR{},hpMemL{},hpMemR{},duckEnv{},admPredL{},admPredR{};
        double phase1=0.0,phase2=0.0,drift=0.0,driftTarget=0.0,admStepL=1.0/128.0,admStepR=1.0/128.0;
        uint32_t rng=0x12345678u;
        void prepare(double sampleRate){sr=std::max(8000.0,sampleRate);const size_t n=(size_t)std::ceil(sr*6.0)+16;l.assign(n,T{});r.assign(n,T{});reset();}
        void reset(){std::fill(l.begin(),l.end(),T{});std::fill(r.begin(),r.end(),T{});w=0;lpL=lpR=hpMemL=hpMemR=duckEnv=admPredL=admPredR=T{};phase1=phase2=drift=driftTarget=0.0;rng=0x12345678u;admStepL=admStepR=1.0/128.0;}
        double rnd(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return (double(rng)/double(UINT32_MAX))*2.0-1.0;}
        static T sat(T x,double drive=1.0,double asym=0.0){const double v=(double)x*drive+asym*0.08;const double y=std::tanh(v)-std::tanh(asym*0.08);return(T)(y/std::max(0.25,std::tanh(std::max(0.25,drive))));}
        T read(const std::vector<T>& b,double ds)const{if(b.empty())return T{};ds=std::clamp(ds,1.0,double(b.size()-4));double p=double(w)-ds;while(p<0)p+=b.size();size_t i=(size_t)p,j=(i+1)%b.size();double f=p-double(i);return(T)((1-f)*double(b[i])+f*double(b[j]));}
        static double lpCoeff(double hz,double sr){return 1.0-std::exp(-2.0*pi*std::clamp(hz,20.0,0.48*sr)/sr);}
        T lowpass(T x,T& z,double hz){double a=lpCoeff(hz,sr);z=(T)(double(z)+a*(double(x)-double(z)));return z;}
        T highpass(T x,T& z,double hz){double a=lpCoeff(hz,sr);z=(T)(double(z)+a*(double(x)-double(z)));return(T)(double(x)-double(z));}
        double commonMod(double depth,double rate,double irregular){phase1+=2*pi*rate/sr;if(phase1>2*pi)phase1-=2*pi;phase2+=2*pi*(rate*3.71+0.13)/sr;if(phase2>2*pi)phase2-=2*pi;if(std::abs(drift-driftTarget)<0.003)driftTarget=rnd();drift+=(driftTarget-drift)*(0.0002+irregular*0.0008);return depth*(0.62*std::sin(phase1)+0.24*std::sin(phase2)+0.34*drift);}
        Stereo digitalColor(Stereo x,int type){if(type==0)return x;if(type==2){double q=2048.0;x.l=(T)(std::round(double(x.l)*q)/q);x.r=(T)(std::round(double(x.r)*q)/q);return x;}auto adm=[&](T in,T& pred,double& step){double d=double(in)-double(pred),sg=d>=0?1.0:-1.0;pred=(T)(double(pred)+sg*step);if(std::abs(d)>step*1.7)step*=1.08;else step*=0.995;step=std::clamp(step,1.0/2048.0,0.10);return pred;};x.l=adm(x.l,admPredL,admStepL);x.r=adm(x.r,admPredR,admStepR);return x;}
        static std::array<double,4> volanteRatios(double spacing){static constexpr double m[4][4]={{.25,.50,.75,1.0},{1.0/6.0,1.0/3.0,2.0/3.0,1.0},{.236,.382,.618,1.0},{.172,.414,.707,1.0}};double x=std::clamp(spacing,0.0,1.0)*3.0;int a=std::min(2,(int)x);double t=x-a;std::array<double,4> z{};for(int i=0;i<4;++i)z[i]=m[a][i]+(m[a+1][i]-m[a][i])*t;return z;}
        Stereo process(T inL,T inR,const SlotParams& p,double bpm){
            if(l.empty())return{};int algo=normIndex(p.algorithm,kAlgorithmCount);auto& c=p.c[algo];double ms=slotDelayMs(p,bpm),fb=std::clamp(p.feedback*1.075,0.0,1.075),level=std::clamp(p.level*1.25,0.0,1.25),duckAmt=std::clamp(p.duck,0.0,1.0);
            double ia=std::max(std::abs((double)inL),std::abs((double)inR)),atk=1.0-std::exp(-1.0/(0.008*sr)),rel=1.0-std::exp(-1.0/(0.180*sr));duckEnv=(T)(double(duckEnv)+(ia-double(duckEnv))*(ia>double(duckEnv)?atk:rel));double duckGain=1.0-duckAmt*std::clamp(double(duckEnv)*2.0,0.0,0.85);Stereo wet{},fbs{};
            if(algo==(int)DelayAlgorithm::Volante){
                ms=std::clamp(ms,100.0,4000.0);double mechanics=c[0],wear=c[1],spread=c[4],drive=c[5],mod=commonMod(0.0015+0.012*mechanics,0.35+0.75*mechanics,mechanics);auto ratios=volanteRatios(c[2]);double sp=0,sf=0;Stereo feedback{};
                for(int h=0;h<4;++h){if(c[6+h]<0.5&&c[10+h]<0.5)continue;double ds=sr*ms*ratios[h]/1000.0*(1.0+mod),mono=.5*(double(read(l,ds))+double(read(r,ds))),pos=std::clamp((p.headPan[h]*2.0-1.0)*spread,-1.0,1.0),gl=std::sqrt(.5*(1-pos)),gr=std::sqrt(.5*(1+pos));if(c[6+h]>=.5){wet.l+=(T)(mono*gl);wet.r+=(T)(mono*gr);sp++;}if(c[10+h]>=.5){feedback.l+=(T)(mono*gl);feedback.r+=(T)(mono*gr);sf++;}}
                if(sp>0){wet.l=(T)(double(wet.l)/std::sqrt(sp));wet.r=(T)(double(wet.r)/std::sqrt(sp));}if(sf>0){feedback.l=(T)(double(feedback.l)/sf);feedback.r=(T)(double(feedback.r)/sf);}double cutoff=11000-7800*wear;fbs.l=lowpass(sat(feedback.l,1+2.2*drive,.08),lpL,cutoff);fbs.r=lowpass(sat(feedback.r,1+2.2*drive,-.05),lpR,cutoff);double lc=35+450*c[3];fbs.l=highpass(fbs.l,hpMemL,lc);fbs.r=highpass(fbs.r,hpMemR,lc);if(wear>.35){double n=rnd()*.0005*wear;fbs.l+=(T)n;fbs.r-=(T)(n*.73);}
            }else if(algo==(int)DelayAlgorithm::ElCapistan){
                ms=std::clamp(ms,35.0,2500.0);double age=c[0],wow=c[1],flutter=c[2],crinkle=c[3],bias=c[4],lowContour=c[5],spring=c[6],mod=commonMod(.003+.018*wow+.006*flutter,.25+5.5*flutter,wow+flutter),base=sr*ms/1000.0*(1+mod);int mode=normIndex(c[7],3);auto tap=[&](double rr){return Stereo{read(l,base*rr),read(r,base*rr)};};if(mode==0)wet=tap(1);else if(mode==1){auto a=tap(.5),b=tap(1);wet={(T)(.64*a.l+.64*b.l),(T)(.64*a.r+.64*b.r)};}else wet=tap(.72+.28*c[7]);if(spring>.001){auto s1=tap(.071),s2=tap(.103);wet.l+=(T)(spring*.25*(double(s1.r)-double(s2.l)));wet.r+=(T)(spring*.25*(double(s1.l)-double(s2.r)));}double cutoff=12500-8500*age;fbs.l=lowpass(sat(wet.l,1.2+2.4*bias,.12),lpL,cutoff);fbs.r=lowpass(sat(wet.r,1.2+2.4*bias,-.08),lpR,cutoff);fbs.l=highpass(fbs.l,hpMemL,45+350*lowContour);fbs.r=highpass(fbs.r,hpMemR,45+350*lowContour);if(crinkle>.03&&rnd()>.992-.010*crinkle){double k=1-.65*crinkle;fbs.l=(T)(double(fbs.l)*k);fbs.r=(T)(double(fbs.r)*k);}double hiss=rnd()*age*.00045;fbs.l+=(T)hiss;fbs.r+=(T)(hiss*.77);
            }else if(algo==(int)DelayAlgorithm::Olivera){
                ms=std::clamp(ms,155.0,620.0);double wear=c[0],visc=c[1],statik=c[2],hm=c[3],tone=c[4],dr=c[5],mod=commonMod(.006+.020*dr,.12+.48*(1-visc),.7+visc),base=sr*ms/1000.0*(1+mod);auto sh=Stereo{read(l,base*.47),read(r,base*.47)},lh=Stereo{read(l,base),read(r,base)},disc=Stereo{read(l,std::min(base*1.29,double(l.size()-4))),read(r,std::min(base*1.29,double(r.size()-4)))};wet.l=(T)((1-hm)*double(sh.l)+hm*double(lh.l)+.18*double(disc.l));wet.r=(T)((1-hm)*double(sh.r)+hm*double(lh.r)+.18*double(disc.r));double cutoff=1800+5200*tone-900*wear;fbs.l=lowpass(sat(wet.l,1.35+1.8*wear,.05),lpL,cutoff);fbs.r=lowpass(sat(wet.r,1.35+1.8*wear,-.04),lpR,cutoff);double res=rnd()*statik*.001;fbs.l+=(T)res;fbs.r-=(T)(res*.6);
            }else if(algo==(int)DelayAlgorithm::EC1){
                ms=std::clamp(ms,40.0,2500.0);double mech=c[0],age=c[1],bias=c[2],pre=c[3],rec=c[4],st=c[5],mod=commonMod(.002+.015*mech,.28+.9*mech,mech),base=sr*ms/1000.0*(1+mod);wet={read(l,base*(1-.0025*st)),read(r,base*(1+.0025*st))};double cutoff=13000-8200*age;fbs.l=lowpass(sat(wet.l,1+2.2*rec+1.2*pre,.10*bias),lpL,cutoff);fbs.r=lowpass(sat(wet.r,1+2.2*rec+1.2*pre,-.07*bias),lpR,cutoff);
            }else if(algo==(int)DelayAlgorithm::Brig){
                int voice=normIndex(c[0],3);bool sync=p.sync>=.5;double maxMs=sync?2000.0:(voice==0?300.0:1000.0),minMs=voice==0?30.0:100.0;ms=std::clamp(ms,minMs,maxMs);double filter=c[1],md=c[2],mr=c[3],comp=c[4],noise=c[5],mod=commonMod((.001+.012*md)*(voice==0?1.25:1),.20+3.6*mr,.2),base=sr*ms/1000.0*(1+mod);if(voice==2){auto a=Stereo{read(l,base),read(r,base*.618)};wet=a;fbs={(T)((1-c[1]*.25)*double(a.r)),(T)((1-c[1]*.25)*double(a.l))};}else{wet={read(l,base),read(r,base)};fbs=wet;}double cutoff=(voice==0?3100.0:voice==1?6200.0:8500.0)*(.55+.75*filter);fbs.l=lowpass(sat(fbs.l,1.25+(voice==0?1.25:.55),.03),lpL,cutoff);fbs.r=lowpass(sat(fbs.r,1.25+(voice==0?1.25:.55),-.03),lpR,cutoff);if(comp>0){double g=1+2.2*comp;fbs.l=(T)(std::copysign(std::pow(std::abs((double)fbs.l),1.0/g),double(fbs.l))*.85);fbs.r=(T)(std::copysign(std::pow(std::abs((double)fbs.r),1.0/g),double(fbs.r))*.85);}double n=rnd()*noise*(voice==0?.0012:.00055);fbs.l+=(T)n;fbs.r-=(T)(n*.81);
            }else if(algo==(int)DelayAlgorithm::Deco){
                ms=std::clamp(ms,.3,500.0);double saturation=c[0],wobble=c[1],blend=c[2],width=c[4],flange=c[5],mod=commonMod(.0004+.020*wobble,.15+.65*wobble,wobble),d=sr*ms/1000.0*(1+mod);int type=normIndex(c[3],3);if(flange>0&&ms<20)d=std::max(1.0,d+sr*.0035*flange*std::sin(phase1*.37));auto lag=Stereo{read(l,d*(1-.002*width)),read(r,d*(1+.002*width))};lag.l=sat(lag.l,1+2.8*saturation,.05);lag.r=sat(lag.r,1+2.8*saturation,-.04);if(type==1){lag.l=(T)-lag.l;lag.r=(T)-lag.r;}if(type==2){T x=lag.r;lag.r=(T)(.25*double(lag.l));lag.l=x;}wet.l=(T)((1-blend)*double(inL)+blend*double(lag.l));wet.r=(T)((1-blend)*double(inR)+blend*double(lag.r));fbs={};fb=0;
            }else{
                ms=std::clamp(ms,20.0,3200.0);int type=normIndex(c[0],3);double ratio=.5+c[1],md=c[2],cross=c[3],dyn=c[4],tone=c[5],mod=commonMod(.0002+.004*md,.25+2.2*md,.05),d1=sr*ms/1000.0*(1+mod),d2=std::clamp(d1*ratio,1.0,double(l.size()-4));Stereo a{read(l,d1),read(r,d1)},b{read(l,d2),read(r,d2)};a=digitalColor(a,type);b=digitalColor(b,type);wet={(T)(.72*double(a.l)+.62*double(b.l)),(T)(.72*double(a.r)+.62*double(b.r))};double dg=1-dyn*std::clamp(double(duckEnv)*1.4,0.0,.65);fbs.l=(T)(dg*((1-cross)*double(wet.l)+cross*double(wet.r)));fbs.r=(T)(dg*((1-cross)*double(wet.r)+cross*double(wet.l)));double cutoff=5500+12500*tone;fbs.l=lowpass(fbs.l,lpL,cutoff);fbs.r=lowpass(fbs.r,lpR,cutoff);
            }
            T recL=sat((T)(double(inL)+fb*double(fbs.l)),1.05,.01),recR=sat((T)(double(inR)+fb*double(fbs.r)),1.05,-.01);l[w]=recL;r[w]=recR;w=(w+1)%l.size();
            double pan=std::clamp(p.pan*2-1.0,-1.0,1.0),gl=std::sqrt(.5*(1-pan))*1.41421356237,gr=std::sqrt(.5*(1+pan))*1.41421356237;wet.l=(T)(double(wet.l)*level*duckGain*gl);wet.r=(T)(double(wet.r)*level*duckGain*gr);if(p.enable<.5)return{};return wet;
        }
    };
    SlotDSP s[2]; double sr=44100.0;
public:
    void prepare(double sampleRate,int=2){sr=sampleRate;for(auto& x:s)x.prepare(sampleRate);}void reset(){for(auto& x:s)x.reset();}
    template<class Sample> void process(Sample** in,Sample** out,int channels,int n,const MXDelayParams& p,double bpm,double& peak){
        double inGain=gainFromNorm(p.inputTrim,-18,18),outGain=gainFromNorm(p.outputTrim,-18,12),mix=std::clamp(p.mix,0.0,1.0),dryG=std::cos(mix*pi*.5),wetG=std::sin(mix*pi*.5);int routing=normIndex(p.routing,3);bool bypass=p.bypass>=.5,trails=p.spill>=.5;peak=0;if(bypass&&!trails){s[0].reset();s[1].reset();}
        for(int i=0;i<n;++i){T inL=(T)((in&&in[0]?in[0][i]:0)*inGain),inR=(channels>1&&in&&in[1])?(T)(in[1][i]*inGain):inL;Stereo a{},b{},wet{};if(routing==(int)DelayRouting::Parallel){a=s[0].process(inL,inR,p.slot[0],bpm);b=s[1].process(inL,inR,p.slot[1],bpm);wet={(T)(double(a.l)+double(b.l)),(T)(double(a.r)+double(b.r))};}else if(routing==(int)DelayRouting::Series){a=s[0].process(inL,inR,p.slot[0],bpm);b=s[1].process((T)(double(inL)+.72*double(a.l)),(T)(double(inR)+.72*double(a.r)),p.slot[1],bpm);wet={(T)(.48*double(a.l)+double(b.l)),(T)(.48*double(a.r)+double(b.r))};}else{a=s[0].process(inL,inL,p.slot[0],bpm);b=s[1].process(inR,inR,p.slot[1],bpm);wet={a.l,b.r};}T yL=(T)(bypass?double(inL):dryG*double(inL)+wetG*double(wet.l)),yR=(T)(bypass?double(inR):dryG*double(inR)+wetG*double(wet.r));yL=(T)(double(yL)*outGain);yR=(T)(double(yR)*outGain);if(out&&out[0])out[0][i]=(Sample)yL;if(channels>1&&out&&out[1])out[1][i]=(Sample)yR;peak=std::max({peak,std::abs((double)yL),std::abs((double)yR)});}
    }
};
} // namespace JerzyAudio
