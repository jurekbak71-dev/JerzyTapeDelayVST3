#pragma once
#include <algorithm>
#include <cmath>
#include "tapedrive_params.h"
namespace JerzyAudio {
// Behavioural opto/tube/transformer model, inspired by T4 memory; not a circuit clone.
// One stereo-linked optical cell; independent audio circuit states per channel.
class OpticalCompressor {
public:
    void prepare(double sr){rate=std::max(8000.0,sr);smoothA=coefficient(.025);hpA=std::exp(-2*pi*55/rate);bassA=std::exp(-2*pi*180/rate);dcA=std::exp(-2*pi*12/rate);toneA=std::exp(-2*pi*std::min(15000.0,rate*.42)/rate);reset();}
    void reset(){amount=colour=enabled=power=fast=memory=exposure=lastGainReduction=0;makeup=mix=1;for(auto& c:audio)c={};}
    void setParameters(const TapeDriveParams& p){
        amountTarget=std::clamp(p.optoAmount,0.,1.);colourTarget=std::clamp(p.optoColor,0.,1.);
        enabledTarget=p.optoBypass>=.5?0.:1.;mixTarget=std::clamp(p.optoMix,0.,1.);
        makeupTarget=std::pow(10.,(-12+24*std::clamp(p.optoMakeup,0.,1.))/20.);
        const double recovery=std::clamp(p.optoRecovery,0.,1.);
        // 50% of the fast cell recovers in about 60ms at the middle setting.
        fastRelease=coefficient(.040+.095*recovery);
        memoryReleaseMin=.35+1.15*recovery;memoryReleaseMax=1.2+4.2*recovery;
        attackA=coefficient(.010);memoryChargeA=coefficient(.22);
        exposureChargeA=coefficient(.6);exposureReleaseA=coefficient(3.0);
        detectorAttackA=coefficient(.003);detectorReleaseA=coefficient(.025);
    }
    double process(const double* input,double* output,int channels){
        amount=smooth(amount,amountTarget);colour=smooth(colour,colourTarget);
        makeup=smooth(makeup,makeupTarget);mix=smooth(mix,mixTarget);enabled=smooth(enabled,enabledTarget);
        double prepared[2]{},detector=0;
        // Feedback detector listens after optical gain, before the makeup amplifier.
        const double feedbackGain=std::pow(10.,-lastGainReduction/20.);
        for(int ch=0;ch<channels;++ch){
            auto& c=audio[ch];const double x=input[ch];
            c.bass=bassA*c.bass+(1-bassA)*x;
            const double iron=soft(x+.26*colour*c.bass,1.+2.8*colour,0.,c.ironPrevious);
            prepared[ch]=x+colour*(iron-x);
            c.detectorLP=hpA*c.detectorLP+(1-hpA)*prepared[ch];
            const double side=(prepared[ch]-.55*c.detectorLP)*feedbackGain;
            detector=std::max(detector,side*side);
        }
        const double da=detector>power?detectorAttackA:detectorReleaseA;
        power=da*power+(1-da)*detector;
        const double db=10*std::log10(std::max(1e-18,power));
        const double threshold=-8.-40.*amount;
        const double over=db-threshold,knee=10.;
        const double overSoft=over<=-knee*.5?0.:(over>=knee*.5?over:(over+knee*.5)*(over+knee*.5)/(2*knee));
        // Feedback slope uses ratio-1, giving the requested equilibrium ratio.
        const double ratio=2.5+7.5*amount;
        const double requested=std::min(48.,overSoft*(ratio-1.)*std::min(1.,amount*25.));
        const double fa=requested>fast?attackA:fastRelease;
        fast=fa*fast+(1-fa)*requested;
        const double load=std::clamp(fast/24.,0.,1.);
        const double ea=load>exposure?exposureChargeA:exposureReleaseA;
        exposure=ea*exposure+(1-ea)*load;
        const double recoverySeconds=memoryReleaseMin+(memoryReleaseMax-memoryReleaseMin)*exposure;
        const double ma=requested>memory?memoryChargeA:coefficient(recoverySeconds);
        memory=ma*memory+(1-ma)*requested;
        lastGainReduction=std::clamp(.55*fast+.45*memory,0.,36.);
        const double gain=std::pow(10.,-lastGainReduction/20.);
        for(int ch=0;ch<channels;++ch){
            auto& c=audio[ch];const double signal=prepared[ch]*gain*makeup;
            // Biased valve curve creates even as well as odd harmonics. Optical
            // exposure increases drive slightly: colour responds to compression history.
            const double bias=.08+.16*colour;
            const double drive=1.+colour*(3.5+1.5*exposure);
            double valve=soft(signal,drive,bias,c.valvePrevious);
            c.dc=dcA*c.dc+(1-dcA)*valve;valve-=c.dc;
            c.tone=toneA*c.tone+(1-toneA)*valve;
            const double coloured=signal+colour*((.65*valve+.35*c.tone)-signal);
            const double wet=enabled*mix;
            output[ch]=input[ch]+wet*(coloured-input[ch]);
        }
        return lastGainReduction*enabled*mix;
    }
private:
    struct AudioState {double bass=0,detectorLP=0,ironPrevious=0,valvePrevious=0,dc=0,tone=0;};
    // First-order antiderivative antialiasing for tanh, with slope normalization.
    // The colour=0 path is exactly transparent and has no half-sample delay.
    static double logCosh(double x){const double a=std::abs(x);return a+std::log1p(std::exp(-2*a))-std::log(2.);}
    static double soft(double x,double drive,double bias,double& previous){
        const double z=x*drive;const double diff=z-previous;
        const double shaped=std::abs(diff)>1e-6?(logCosh(z+bias)-logCosh(previous+bias))/diff:std::tanh(.5*(z+previous)+bias);
        previous=z;const double t=std::tanh(bias);
        return (shaped-t)/(drive*(1-t*t));
    }
    double coefficient(double seconds)const{return std::exp(-1./(seconds*rate));}
    double smooth(double x,double target)const{return smoothA*x+(1-smoothA)*target;}
    static constexpr double pi=3.14159265358979323846;
    double rate=44100,smoothA=0,hpA=0,bassA=0,dcA=0,toneA=0;
    double amount=0,colour=0,enabled=0,makeup=1,mix=1;
    double amountTarget=0,colourTarget=0,enabledTarget=0,makeupTarget=1,mixTarget=1;
    double power=0,fast=0,memory=0,exposure=0,lastGainReduction=0;
    double fastRelease=0,memoryReleaseMin=0,memoryReleaseMax=0,attackA=0,memoryChargeA=0;
    double exposureChargeA=0,exposureReleaseA=0,detectorAttackA=0,detectorReleaseA=0;
    AudioState audio[2]{};
};
}
