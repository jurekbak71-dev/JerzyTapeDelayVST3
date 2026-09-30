#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

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
        wowPhase=0.0;
        flutterPhase=0.0;
        flutter2Phase=1.7;
    }

    void process(const Sample* const* in, Sample* const* out, int channels, int n,
                 double sat, double levelNorm, double dryNorm, int gainMode, int shiftMode,
                 double hpfNorm, double hpfResNorm, double lpfNorm, double lpfResNorm,
                 double wowFlutterNorm, int preampMode, double preampDriveNorm,
                 double& inputPeak, double& saturationPeak, double& outputPeak) {
        channels=std::clamp(channels,1,numChannels);
        sat=std::clamp(sat,0.0,1.0);
        levelNorm=std::clamp(levelNorm,0.0,1.0);
        dryNorm=std::clamp(dryNorm,0.0,1.0);
        wowFlutterNorm=std::clamp(wowFlutterNorm,0.0,1.0);
        preampDriveNorm=std::clamp(preampDriveNorm,0.0,1.0);
        preampMode=std::clamp(preampMode,0,2);

        inputPeak=0.0; saturationPeak=0.0; outputPeak=0.0;

        const double driveDb = (gainMode ? 12.0 + 30.0*sat : 22.0*sat);
        const double drive = std::pow(10.0,driveDb/20.0);
        const double outDb = -24.0 + 36.0*levelNorm;
        const double outGain = std::pow(10.0,outDb/20.0);
        const double dryGain = dryNorm <= 0.001 ? 0.0 : std::pow(10.0,(6.0*dryNorm)/20.0)*dryNorm;
        const double lowA = std::exp(-2.0*3.141592653589793*2200.0/sampleRate);
        const double atk = std::exp(-1.0/(0.006*sampleRate));
        const double rel = std::exp(-1.0/(0.090*sampleRate));
        const double dcA = std::exp(-2.0*3.141592653589793*18.0/sampleRate);

        Biquad hp = makeHighPass(mapLog(hpfNorm,20.0,2000.0),mapQ(hpfResNorm));
        Biquad lp = makeLowPass(mapLog(lpfNorm,1000.0,20000.0),mapQ(lpfResNorm));

        const double wearCutoff=18000.0-10500.0*wowFlutterNorm;
        const double wearA=std::exp(-2.0*3.141592653589793*wearCutoff/sampleRate);
        const double wowInc=2.0*3.141592653589793*0.33/sampleRate;
        const double flutterInc=2.0*3.141592653589793*5.7/sampleRate;
        const double flutter2Inc=2.0*3.141592653589793*11.1/sampleRate;

        const double tubeDrive=std::pow(10.0,(4.0+30.0*preampDriveNorm)/20.0);
        const double transistorDrive=std::pow(10.0,(2.0+34.0*preampDriveNorm)/20.0);
        const double tubeSagAtk=std::exp(-1.0/(0.018*sampleRate));
        const double tubeToneA=std::exp(-2.0*3.141592653589793*(11500.0-3500.0*preampDriveNorm)/sampleRate);
        const double transistorToneA=std::exp(-2.0*3.141592653589793*4200.0/sampleRate);

        for(int i=0;i<n;++i){
            const double modulation =
                wowFlutterNorm*(2.8*std::sin(wowPhase)
                              +0.55*std::sin(flutterPhase)
                              +0.20*std::sin(flutter2Phase));
            const double baseDelayMs = wowFlutterNorm>0.0001 ? 4.2 : 0.0;

            for(int ch=0;ch<channels;++ch){
                const double x = in && in[ch] ? static_cast<double>(in[ch][i]) : 0.0;
                inputPeak=std::max(inputPeak,std::abs(x));

                // PREAMP STAGE: OFF / TUBE / TRANSISTOR
                double pre=x;
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

                wet*=outGain;
                dc[ch]=dcA*dc[ch]+(1.0-dcA)*wet;
                wet-=dc[ch];

                // DRY stays true dry, preamp/tape are in wet path
                double y=wet+x*dryGain;

                if(wowFlutterNorm>0.0001 && !wowBuffer[ch].empty()){
                    const double stereoSkew=(ch==0?-0.08:0.08)*wowFlutterNorm*std::sin(flutter2Phase);
                    const double delayMs=std::max(0.6,baseDelayMs+modulation+stereoSkew);
                    const double delaySamples=delayMs*0.001*sampleRate;
                    const double delayed=readDelay(ch,y,delaySamples);
                    const double blend=0.18+0.72*wowFlutterNorm;
                    y=y*(1.0-blend)+delayed*blend;

                    wearLP[ch]=(1.0-wearA)*y+wearA*wearLP[ch];
                    y=y*(1.0-0.42*wowFlutterNorm)+wearLP[ch]*(0.42*wowFlutterNorm);
                }

                y=runBiquad(y,hp,hp_z1[ch],hp_z2[ch]);
                y=runBiquad(y,lp,lp_z1[ch],lp_z2[ch]);

                outputPeak=std::max(outputPeak,std::abs(y));
                if(out && out[ch]) out[ch][i]=static_cast<Sample>(y);
            }

            wowPhase=wrapPhase(wowPhase+wowInc);
            flutterPhase=wrapPhase(flutterPhase+flutterInc);
            flutter2Phase=wrapPhase(flutter2Phase+flutter2Inc);
        }
    }

private:
    struct Biquad { double b0{},b1{},b2{},a1{},a2{}; };

    static double wrapPhase(double p){
        const double twoPi=6.2831853071795864769;
        return p>=twoPi?p-twoPi:p;
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

    double readDelay(int ch,double input,double delaySamples){
        auto& b=wowBuffer[ch];
        if(b.empty()) return input;
        const size_t size=b.size();
        b[wowWrite[ch]]=input;
        double rp=static_cast<double>(wowWrite[ch])-delaySamples;
        while(rp<0.0) rp+=static_cast<double>(size);
        while(rp>=static_cast<double>(size)) rp-=static_cast<double>(size);
        const size_t i0=static_cast<size_t>(rp);
        const size_t i1=(i0+1)%size;
        const double frac=rp-static_cast<double>(i0);
        const double y=b[i0]*(1.0-frac)+b[i1]*frac;
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
    double wowPhase=0.0,flutterPhase=0.0,flutter2Phase=1.7;
};

}
