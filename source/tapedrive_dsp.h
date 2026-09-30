#pragma once
#include <algorithm>
#include <cmath>

namespace JerzyAudio {

template <typename Sample>
class TapeDriveDSP {
public:
    void prepare(double sr, int channels=2) {
        sampleRate=std::max(8000.0,sr);
        numChannels=std::clamp(channels,1,2);
        reset();
    }

    void reset(){
        for(int i=0;i<2;++i){
            low[i]=0.0; env[i]=0.0; dc[i]=0.0;
            hp_z1[i]=hp_z2[i]=0.0;
            lp_z1[i]=lp_z2[i]=0.0;
        }
    }

    void process(const Sample* const* in, Sample* const* out, int channels, int n,
                 double sat, double levelNorm, double dryNorm, int gainMode, int shiftMode,
                 double hpfNorm, double hpfResNorm, double lpfNorm, double lpfResNorm) {
        channels=std::clamp(channels,1,numChannels);
        sat=std::clamp(sat,0.0,1.0);
        levelNorm=std::clamp(levelNorm,0.0,1.0);
        dryNorm=std::clamp(dryNorm,0.0,1.0);

        const double driveDb = (gainMode ? 12.0 + 30.0*sat : 22.0*sat);
        const double drive = std::pow(10.0,driveDb/20.0);
        const double outDb = -24.0 + 36.0*levelNorm;
        const double outGain = std::pow(10.0,outDb/20.0);
        const double dryGain = dryNorm <= 0.001 ? 0.0 : std::pow(10.0, (6.0*dryNorm)/20.0) * dryNorm;
        const double lowA = std::exp(-2.0*3.141592653589793*2200.0/sampleRate);
        const double atk = std::exp(-1.0/(0.006*sampleRate));
        const double rel = std::exp(-1.0/(0.090*sampleRate));
        const double dcA = std::exp(-2.0*3.141592653589793*18.0/sampleRate);

        Biquad hp = makeHighPass(mapLog(hpfNorm,20.0,2000.0), mapQ(hpfResNorm));
        Biquad lp = makeLowPass(mapLog(lpfNorm,1000.0,20000.0), mapQ(lpfResNorm));

        for(int i=0;i<n;++i){
            for(int ch=0;ch<channels;++ch){
                const double x = in && in[ch] ? (double)in[ch][i] : 0.0;
                low[ch] = (1.0-lowA)*x + lowA*low[ch];
                const double high = x-low[ch];

                double shapedIn=x;
                if(shiftMode==1) shapedIn = x + 0.34*high;
                else if(shiftMode==2) shapedIn = x + 0.18*high + 0.10*low[ch];

                const double a=std::abs(shapedIn);
                const double coeff = a>env[ch] ? atk : rel;
                env[ch] = coeff*env[ch] + (1.0-coeff)*a;
                const double comp = 1.0/(1.0 + env[ch]*(0.8 + 2.7*sat));

                const double bias = 0.035 + 0.075*sat;
                const double z = shapedIn*drive*comp;
                double wet = std::tanh(z + bias) - std::tanh(bias);
                wet += 0.08*sat*std::tanh(z*z*(z>=0?1.0:-1.0));
                wet *= outGain;

                dc[ch] = dcA*dc[ch] + (1.0-dcA)*wet;
                wet -= dc[ch];

                double y = wet + x*dryGain;
                y = runBiquad(y,hp,hp_z1[ch],hp_z2[ch]);
                y = runBiquad(y,lp,lp_z1[ch],lp_z2[ch]);

                if(out && out[ch]) out[ch][i]=(Sample)y;
            }
        }
    }

private:
    struct Biquad { double b0{},b1{},b2{},a1{},a2{}; };

    static double mapLog(double n,double lo,double hi){
        n=std::clamp(n,0.0,1.0);
        return lo*std::pow(hi/lo,n);
    }
    static double mapQ(double n){
        n=std::clamp(n,0.0,1.0);
        return 0.5 + 11.5*n;
    }

    Biquad makeLowPass(double freq,double q) const {
        return makeFilter(freq,q,false);
    }
    Biquad makeHighPass(double freq,double q) const {
        return makeFilter(freq,q,true);
    }

    Biquad makeFilter(double freq,double q,bool highPass) const {
        const double pi=3.1415926535897932384626433832795;
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

    double sampleRate=44100.0;
    int numChannels=2;
    double low[2]{}, env[2]{}, dc[2]{};
    double hp_z1[2]{},hp_z2[2]{},lp_z1[2]{},lp_z2[2]{};
};

}
