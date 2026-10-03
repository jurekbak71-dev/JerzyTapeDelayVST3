#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdint>
#include <atomic>
#include <chrono>
#include "tapedrive_params.h"
#include "optical_compressor.h"

namespace JerzyAudio {

template <typename Sample>
class TapeDriveDSP {
public:
    void prepare(double sr, int channels=2) {
        sampleRate=std::max(8000.0,sr);
        numChannels=std::clamp(channels,1,2);
        const size_t delaySize=static_cast<size_t>(sampleRate*0.040)+8;
        for(int ch=0;ch<2;++ch){
            wowBuffer[ch].assign(delaySize,0.0);
            wowWrite[ch]=0;
        }
        static std::atomic<uint32_t> seedCounter{0x9e3779b9u};
        randomState=static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count()) ^ seedCounter.fetch_add(0x9e3779b9u);
        if(!randomState)randomState=0x4a65727au;
        opto.prepare(sampleRate);
        reset();
    }

    void reset(){
        for(int i=0;i<2;++i){
            low[i]=0.0; env[i]=0.0; dc[i]=0.0;
            hp_z1[i]=hp_z2[i]=0.0;
            lp_z1[i]=lp_z2[i]=0.0;
            wearLP[i]=0.0;
            tubeSag[i]=0.0;
            tubeTone[i]=0.0;
            transistorTone[i]=0.0;
            std::fill(wowBuffer[i].begin(),wowBuffer[i].end(),0.0);
            wowWrite[i]=0;
        }
        lastDelaySamples=0.0;
        wowMotion={}; flutterMotion={}; driftMotion={}; crinkleMotion={};
        crinkleEvent={};
        smoothTransport=0.0;
        smoothWow=smoothFlutter=smoothAge=0.0;
        opto.reset();
        for(int ch=0;ch<2;++ch){
            hissLP[ch]=crackle[ch]=dropout[ch]=dropoutTarget[ch]=0.0;
            noiseBreath[ch]=0.0;
            dropoutLeft[ch]=0;
        }
    }

    // Test/offline reproducibility only. Normal playback gets a fresh seed on prepare.
    void setRandomSeed(uint32_t seed){randomState=seed?seed:0x4a65727au;}

    void process(const Sample* const* in, Sample* const* out, int channels, int n,
                 const TapeDriveParams& p,
                 double& inputPeak, double& saturationPeak, double& outputPeak,
                 double& reductionPeak) {
        double sat=p.sat, levelNorm=p.level, dryNorm=p.dry;
        const int gainMode=p.gainMode>=0.5?1:0;
        const int shiftMode=std::clamp(static_cast<int>(std::lround(p.shift*2.0)),0,2);
        int preampMode=std::clamp(static_cast<int>(std::lround(p.preampMode*2.0)),0,2);
        double preampDriveNorm=p.preampDrive;
        const double wowTarget=std::clamp(p.wowFlutter,0.0,1.0);
        const double flutterTarget=std::clamp(p.flutter,0.0,1.0);
        const double ageTarget=std::clamp(p.tapeAge,0.0,1.0);
        channels=std::clamp(channels,1,numChannels);
        sat=std::clamp(sat,0.0,1.0);
        levelNorm=std::clamp(levelNorm,0.0,1.0);
        dryNorm=std::clamp(dryNorm,0.0,1.0);

        preampDriveNorm=std::clamp(preampDriveNorm,0.0,1.0);
        preampMode=std::clamp(preampMode,0,2);

        inputPeak=0.0; saturationPeak=0.0; outputPeak=0.0; reductionPeak=0.0;

        const double driveDb = (gainMode ? 12.0 + 30.0*sat : 22.0*sat);
        const double drive = std::pow(10.0,driveDb/20.0);
        const double outDb = -24.0 + 36.0*levelNorm;
        const double outGain = std::pow(10.0,outDb/20.0);
        const double dryGain = dryNorm <= 0.001 ? 0.0 : std::pow(10.0,(6.0*dryNorm)/20.0)*dryNorm;
        const double lowA = std::exp(-2.0*3.141592653589793*2200.0/sampleRate);
        const double atk = std::exp(-1.0/(0.006*sampleRate));
        const double rel = std::exp(-1.0/(0.090*sampleRate));
        const double dcA = std::exp(-2.0*3.141592653589793*18.0/sampleRate);

        Biquad hp = makeHighPass(mapLog(p.hpfCutoff,20.0,2000.0),mapQ(p.hpfRes));
        Biquad lp = makeLowPass(mapLog(p.lpfCutoff,1000.0,20000.0),mapQ(p.lpfRes));

        const double smoothA=std::exp(-1.0/(0.025*sampleRate));
        const double hissA=std::exp(-2.0*3.141592653589793*1600.0/sampleRate);
        const double crackA=std::exp(-1.0/(0.0007*sampleRate));
        const double dropAttack=std::exp(-1.0/(0.0018*sampleRate));
        const double dropRelease=std::exp(-1.0/(0.025*sampleRate));
        const double breathA=std::exp(-1.0/(0.060*sampleRate));
        const double transportTarget=std::max({wowTarget,flutterTarget,ageTarget})>1e-8?1.0:0.0;

        const double tubeDrive=std::pow(10.0,(4.0+30.0*preampDriveNorm)/20.0);
        const double transistorDrive=std::pow(10.0,(2.0+34.0*preampDriveNorm)/20.0);
        const double tubeSagAtk=std::exp(-1.0/(0.018*sampleRate));
        const double tubeToneA=std::exp(-2.0*3.141592653589793*(11500.0-3500.0*preampDriveNorm)/sampleRate);
        const double transistorToneA=std::exp(-2.0*3.141592653589793*4200.0/sampleRate);

        opto.setParameters(p);
        for(int i=0;i<n;++i){
            smoothWow=smoothA*smoothWow+(1.0-smoothA)*wowTarget;
            smoothFlutter=smoothA*smoothFlutter+(1.0-smoothA)*flutterTarget;
            smoothAge=smoothA*smoothAge+(1.0-smoothA)*ageTarget;
            smoothTransport=smoothA*smoothTransport+(1.0-smoothA)*transportTarget;
            // Several time scales, independently drawn durations and amplitudes.
            // Age also agitates the transport: physical damage changes pitch and head contact.
            const double wow=nextMotion(wowMotion,0.10,0.85);
            const double drift=nextMotion(driftMotion,0.8,3.8);
            const double flutter=nextMotion(flutterMotion,0.005,0.028);
            const double crinkle=nextMotion(crinkleMotion,0.002,0.009);
            const double damage=nextCrinkle(smoothAge);
            const double wowDepth=smoothWow*(0.20+0.80*smoothWow);
            const double flutterDepth=smoothFlutter*(0.20+0.80*smoothFlutter);
            const double age2=smoothAge*smoothAge;
            const double delayMs=16.0*smoothTransport
                +8.5*wowDepth*wow+1.5*wowDepth*drift
                +0.85*flutterDepth*flutter
                +1.1*age2*drift+damage*(2.6*crinkleEvent.sign+0.35*crinkle);
            const double desiredDelay=std::clamp(delayMs,0.0,36.0)*0.001*sampleRate;
            // Bound read-head velocity: extreme simultaneous controls cannot reverse playback.
            lastDelaySamples+=std::clamp(desiredDelay-lastDelaySamples,-0.65,0.65);
            const double delaySamples=lastDelaySamples;
            double opticalInput[2]{},opticalOutput[2]{};
            for(int ch=0;ch<channels;++ch)
                opticalInput[ch]=in && in[ch]?static_cast<double>(in[ch][i]):0.0;
            const double gr=opto.process(opticalInput,opticalOutput,channels);
            reductionPeak=std::max(reductionPeak,gr/36.0);

            for(int ch=0;ch<channels;++ch){
                const double x = in && in[ch] ? static_cast<double>(in[ch][i]) : 0.0;
                inputPeak=std::max(inputPeak,std::abs(x));

                // PREAMP STAGE: OFF / TUBE / TRANSISTOR
                double pre=opticalOutput[ch];
                double preSat=0.0;
                if(preampMode==1){
                    // tube: soft asymmetry, even harmonics, sag and gentle HF rolloff
                    const double absx=std::abs(pre);
                    tubeSag[ch]=tubeSagAtk*tubeSag[ch]+(1.0-tubeSagAtk)*absx;
                    const double sag=1.0/(1.0+tubeSag[ch]*(0.55+1.8*preampDriveNorm));
                    const double z=pre*tubeDrive*sag;
                    const double bias=0.10+0.12*preampDriveNorm;
                    double y=std::tanh(z+bias)-std::tanh(bias);
                    y+=0.10*preampDriveNorm*(y*y)*(y>=0.0?1.0:-0.45);
                    y*=0.78/std::max(0.45,std::sqrt(tubeDrive)*0.22);
                    tubeTone[ch]=(1.0-tubeToneA)*y+tubeToneA*tubeTone[ch];
                    pre=0.72*y+0.28*tubeTone[ch];
                    preSat=std::clamp(std::abs(z-y)/(std::abs(z)+0.25),0.0,1.0);
                }else if(preampMode==2){
                    // transistor: tighter, faster, more odd harmonics and harder knee
                    transistorTone[ch]=(1.0-transistorToneA)*pre+transistorToneA*transistorTone[ch];
                    const double presence=pre-transistorTone[ch];
                    const double z=(pre+0.18*presence)*transistorDrive;
                    const double soft=std::tanh(z*1.15);
                    const double hard=std::clamp(z,-1.15,0.92);
                    pre=(0.48*soft+0.52*hard)/(0.75+0.18*preampDriveNorm);
                    pre+=0.045*preampDriveNorm*pre*pre*pre;
                    preSat=std::clamp(std::abs(z-pre)/(std::abs(z)+0.22),0.0,1.0);
                }

                low[ch]=(1.0-lowA)*pre+lowA*low[ch];
                const double high=pre-low[ch];

                double shapedIn=pre;
                if(shiftMode==1) shapedIn=pre+0.34*high;
                else if(shiftMode==2) shapedIn=pre+0.18*high+0.10*low[ch];

                const double a=std::abs(shapedIn);
                const double coeff=a>env[ch]?atk:rel;
                env[ch]=coeff*env[ch]+(1.0-coeff)*a;
                const double comp=1.0/(1.0+env[ch]*(0.8+2.7*sat));

                const double bias=0.035+0.075*sat;
                const double z=shapedIn*drive*comp;
                const double soft=std::tanh(z+bias)-std::tanh(bias);
                double wet=soft+0.08*sat*std::tanh(z*z*(z>=0?1.0:-1.0));

                const double tapeSat=std::clamp(std::abs(z-soft)/(std::abs(z)+0.35),0.0,1.0)
                                     *std::min(1.0,0.25+1.15*sat);
                saturationPeak=std::max(saturationPeak,std::max(preSat,tapeSat));

                dc[ch]=dcA*dc[ch]+(1.0-dcA)*wet;
                wet-=dc[ch];

                // Always write transport history, including at zero modulation.
                wet=readDelay(ch,wet,std::max(0.0,delaySamples));
                if(dropoutLeft[ch]>0){
                    --dropoutLeft[ch];
                }else{
                    dropoutTarget[ch]=0.0;
                    // Damage occurs in bursts. At full Age there are several events per second.
                    const double rate=age2*(0.4+5.5*smoothAge+9.0*damage);
                    if(smoothAge>0.0001 && uniform()<rate/sampleRate){
                        dropoutTarget[ch]=(0.25+0.73*uniform())*smoothAge;
                        dropoutLeft[ch]=static_cast<int>((0.006+0.23*uniform()*uniform())*sampleRate);
                    }
                }
                const double dropA=dropoutTarget[ch]>dropout[ch]?dropAttack:dropRelease;
                dropout[ch]=dropA*dropout[ch]+(1.0-dropA)*dropoutTarget[ch];
                // Variable spacing loss/azimuth approximation, rather than a fixed dark EQ.
                const double contactLoss=std::clamp(0.20*age2+0.65*dropout[ch]+0.30*damage,0.0,1.0);
                const double wearCutoff=18000.0*std::pow(850.0/18000.0,contactLoss);
                const double wearA=std::exp(-2.0*3.141592653589793*wearCutoff/sampleRate);
                wearLP[ch]=(1.0-wearA)*wet+wearA*wearLP[ch];
                const double darkMix=std::min(1.0,0.65*smoothAge+0.6*damage+dropout[ch]);
                wet=wet*(1.0-darkMix)+wearLP[ch]*darkMix;
                wet*=std::max(0.015,1.0-dropout[ch]-0.12*damage);
                const double noise=2.0*uniform()-1.0;
                hissLP[ch]=hissA*hissLP[ch]+(1.0-hissA)*noise;
                noiseBreath[ch]=breathA*noiseBreath[ch]+(1.0-breathA)*std::min(1.0,std::abs(wet)*4.0);
                const double hiss=age2*0.017*(1.0+1.5*damage+0.6*noiseBreath[ch])*(noise-0.75*hissLP[ch]);
                crackle[ch]*=crackA;
                const double crackRate=age2*(1.0+15.0*smoothAge+85.0*damage);
                if(smoothAge>0.0001 && uniform()<crackRate/sampleRate)
                    crackle[ch]+=(2.0*uniform()-1.0)*(0.035+0.14*damage)*smoothAge;
                wet+=hiss+crackle[ch];
                wet*=outGain;
                // The existing additive DRY control stays unprocessed.
                double y=wet+x*dryGain;

                y=runBiquad(y,hp,hp_z1[ch],hp_z2[ch]);
                y=runBiquad(y,lp,lp_z1[ch],lp_z2[ch]);

                outputPeak=std::max(outputPeak,std::abs(y));
                if(out && out[ch]) out[ch][i]=static_cast<Sample>(y);
            }

        }
    }

private:
    struct Biquad { double b0{},b1{},b2{},a1{},a2{}; };

    struct Motion { double from=0.0,to=0.0; int elapsed=0,length=0; };
    double uniform(){
        // Local PRNG: no locks, allocations or global RNG in the audio callback.
        randomState^=randomState<<13; randomState^=randomState>>17; randomState^=randomState<<5;
        return static_cast<double>(randomState)/4294967296.0;
    }
    double nextMotion(Motion& motion,double minSeconds,double maxSeconds){
        if(motion.elapsed>=motion.length){
            motion.from=motion.to;
            motion.to=2.0*uniform()-1.0;
            motion.length=std::max(1,static_cast<int>((minSeconds+(maxSeconds-minSeconds)*uniform())*sampleRate));
            motion.elapsed=0;
        }
        const double t=static_cast<double>(motion.elapsed++)/motion.length;
        return motion.from+(motion.to-motion.from)*t*t*(3.0-2.0*t);
    }
    static double mapLog(double n,double lo,double hi){
        n=std::clamp(n,0.0,1.0);
        return lo*std::pow(hi/lo,n);
    }
    static double mapQ(double n){
        n=std::clamp(n,0.0,1.0);
        return 0.5+11.5*n;
    }

    Biquad makeLowPass(double freq,double q) const { return makeFilter(freq,q,false); }
    Biquad makeHighPass(double freq,double q) const { return makeFilter(freq,q,true); }

    Biquad makeFilter(double freq,double q,bool highPass) const {
        const double pi=3.14159265358979323846;
        freq=std::clamp(freq,10.0,sampleRate*0.45);
        q=std::clamp(q,0.5,12.0);
        const double w0=2.0*pi*freq/sampleRate;
        const double c=std::cos(w0);
        const double s=std::sin(w0);
        const double alpha=s/(2.0*q);
        double b0,b1,b2;
        if(highPass){
            b0=(1.0+c)*0.5; b1=-(1.0+c); b2=(1.0+c)*0.5;
        }else{
            b0=(1.0-c)*0.5; b1=1.0-c; b2=(1.0-c)*0.5;
        }
        const double a0=1.0+alpha;
        Biquad b;
        b.b0=b0/a0; b.b1=b1/a0; b.b2=b2/a0;
        b.a1=(-2.0*c)/a0; b.a2=(1.0-alpha)/a0;
        return b;
    }

    static double runBiquad(double x,const Biquad& b,double& z1,double& z2){
        const double y=b.b0*x+z1;
        z1=b.b1*x-b.a1*y+z2;
        z2=b.b2*x-b.a2*y;
        return y;
    }

    struct DamageEvent {int left=0,length=1;double depth=0.0,sign=1.0;};
    double nextCrinkle(double age){
        if(crinkleEvent.left==0){
            if(age<0.0001 || uniform()>age*age*(0.25+7.0*age)/sampleRate)return 0.0;
            crinkleEvent.length=std::max(1,static_cast<int>((0.025+0.22*uniform())*sampleRate));
            crinkleEvent.left=crinkleEvent.length;
            crinkleEvent.depth=age*(0.35+0.65*uniform());
            crinkleEvent.sign=uniform()<0.5?-1.0:1.0;
        }
        const double t=1.0-static_cast<double>(crinkleEvent.left--)/crinkleEvent.length;
        const double ramp=t<0.3?t/0.3:(1.0-t)/0.7;
        const double u=std::clamp(ramp,0.0,1.0);
        return crinkleEvent.depth*u*u*(3.0-2.0*u);
    }
    double readDelay(int ch,double input,double delaySamples){
        auto& b=wowBuffer[ch];
        if(b.empty()) return input;
        const size_t size=b.size();
        b[wowWrite[ch]]=input;
        // Causal cubic interpolation: no unwritten next sample at near-zero delay.
        const size_t delayInt=static_cast<size_t>(delaySamples);
        const double t=delaySamples-delayInt;
        const size_t k=(wowWrite[ch]+size-delayInt)%size;
        const double a=b[k],c=b[(k+size-1)%size],d=b[(k+size-2)%size],e=b[(k+size-3)%size];
        const double y=a*((1-t)*(2-t)*(3-t)/6.)+c*(t*(2-t)*(3-t)/2.)
                      -d*(t*(1-t)*(3-t)/2.)+e*(t*(1-t)*(2-t)/6.);
        wowWrite[ch]=(wowWrite[ch]+1)%size;
        return y;
    }

    double sampleRate=44100.0;
    int numChannels=2;
    double low[2]{},env[2]{},dc[2]{};
    double hp_z1[2]{},hp_z2[2]{},lp_z1[2]{},lp_z2[2]{};
    double wearLP[2]{};
    double tubeSag[2]{},tubeTone[2]{},transistorTone[2]{};
    std::vector<double> wowBuffer[2];
    size_t wowWrite[2]{};
    uint32_t randomState=0x4a65727au;
    Motion wowMotion{},flutterMotion{},driftMotion{},crinkleMotion{};
    DamageEvent crinkleEvent{};
    double smoothTransport=0.0,lastDelaySamples=0.0;
    OpticalCompressor opto;
    double smoothWow=0.0,smoothFlutter=0.0,smoothAge=0.0;

    double hissLP[2]{},crackle[2]{},dropout[2]{},dropoutTarget[2]{},noiseBreath[2]{};
    int dropoutLeft[2]{};
};

}
