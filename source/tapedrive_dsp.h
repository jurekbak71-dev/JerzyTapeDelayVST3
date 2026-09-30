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
        for(int i=0;i<2;++i){ low[i]=0.0; env[i]=0.0; dc[i]=0.0; }
    }
    void reset(){ for(int i=0;i<2;++i){ low[i]=0.0; env[i]=0.0; dc[i]=0.0; } }

    void process(const Sample* const* in, Sample* const* out, int channels, int n,
                 double sat, double levelNorm, double dryNorm, int gainMode, int shiftMode) {
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

        for(int i=0;i<n;++i){
            for(int ch=0;ch<channels;++ch){
                const double x = in && in[ch] ? (double)in[ch][i] : 0.0;
                low[ch] = (1.0-lowA)*x + lowA*low[ch];
                const double high = x-low[ch];

                double shapedIn=x;
                if(shiftMode==1) shapedIn = x + 0.34*high;       // high sparkle
                else if(shiftMode==2) shapedIn = x + 0.18*high + 0.10*low[ch]; // bite/full mids

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

                if(out && out[ch]) out[ch][i]=(Sample)(wet + x*dryGain);
            }
        }
    }
private:
    double sampleRate=44100.0;
    int numChannels=2;
    double low[2]{}, env[2]{}, dc[2]{};
};

}
