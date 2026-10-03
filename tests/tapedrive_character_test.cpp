#include "../source/tapedrive_dsp.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace JerzyAudio;
constexpr double pi=3.14159265358979323846,sr=48000;
void need(bool b,const char* message){if(!b)throw std::runtime_error(message);}
struct Result {std::vector<double> signal,gr;};
Result optical(TapeDriveParams p,double amplitude=.5,double duration=4,double burst=0){
    OpticalCompressor compressor;compressor.prepare(sr);compressor.setParameters(p);
    Result result;for(int i=0;i<int(sr*duration);++i){
        double a=burst>0?(i<sr*burst?.8:.0001):amplitude;
        double input[2]={a*std::sin(2*pi*1000*i/sr),a*std::sin(2*pi*1000*i/sr)},output[2]{};
        double gr=compressor.process(input,output,2);need(std::isfinite(output[0]),"Opto unstable");
        need(output[0]==output[1],"Opto moves the stereo centre");result.signal.push_back(output[0]);result.gr.push_back(gr);
    }return result;
}
double power(const std::vector<double>& v){double x=0;for(size_t i=v.size()/2;i<v.size();++i)x+=v[i]*v[i];return x/(v.size()-v.size()/2);}
double harmonic(const std::vector<double>& v,int harmonic){double re=0,im=0;size_t start=v.size()/2;for(size_t i=start;i<v.size();++i){double a=2*pi*1000*harmonic*i/sr;re+=v[i]*std::cos(a);im+=v[i]*std::sin(a);}return 2*std::hypot(re,im)/(v.size()-start);}
std::vector<double> tape(double wow,double flutter,double age){
    TapeDriveParams p;p.optoBypass=1;p.sat=0;p.shift=0;p.level=2./3.;p.wowFlutter=wow;p.flutter=flutter;p.tapeAge=age;
    TapeDriveDSP<double> dsp;dsp.prepare(sr);dsp.setRandomSeed(0x4a65727au);
    int n=int(sr*12);std::vector<double> input(n),output(n),right(n);
    for(int i=0;i<n;++i)input[i]=.4*std::sin(2*pi*997*i/sr);
    const double* in[]={input.data(),input.data()};double* out[]={output.data(),right.data()};double a,b,c,d;
    dsp.process(in,out,2,n,p,a,b,c,d);return output;
}
std::vector<double> pitch(const std::vector<double>& v){
    std::vector<double> result;double last=-1;
    for(size_t i=int(sr);i<v.size();++i)if(v[i-1]<0&&v[i]>=0){double t=i-1-v[i-1]/(v[i]-v[i-1]);if(last>0)result.push_back(1200*std::log2(sr/(t-last)/997));last=t;}
    return result;
}
double percentileAbs(std::vector<double> v,double percentile){for(auto& x:v)x=std::abs(x);std::sort(v.begin(),v.end());return v[size_t(percentile*(v.size()-1))];}
double windowRMS(const std::vector<double>& v,size_t i,size_t n){double e=0;for(size_t k=i;k<i+n;++k)e+=v[k]*v[k];return std::sqrt(e/n);}
int main(){try{
    const auto clean=tape(0,0,0),wow=tape(1,0,0),flutter=tape(0,1,0),aged=tape(0,0,1);
    const double wowPitch=percentileAbs(pitch(wow),.95),flutterPitch=percentileAbs(pitch(flutter),.95);
    need(wowPitch>25,"Wow max remains too subtle");need(flutterPitch>35,"Flutter max remains too subtle");
    int deepDrops=0;double worstGain=1;for(size_t i=int(sr);i+480<aged.size();i+=480){double ratio=windowRMS(aged,i,480)/windowRMS(clean,i,480);if(ratio<.6)++deepDrops;worstGain=std::min(worstGain,ratio);}
    need(deepDrops>20,"Age must produce many audible head-contact losses");need(worstGain<.35,"Age does not produce deep dropouts");
    // Avoid a regular fixed-period vibrato: adjacent seconds must differ.
    const auto wp=pitch(wow);double motionChange=0;for(size_t i=1000;i<wp.size();++i)motionChange+=std::abs(wp[i]-wp[i-997]);need(motionChange/(wp.size()-1000)>10,"Wow repeats once per second");
    TapeDriveParams p;p.optoColor=0;p.optoAmount=.95;
    auto compressed=optical(p);need(compressed.gr.back()>20,"Opto high setting must give deep reduction");
    p.optoAmount=0;auto neutral=optical(p);need(std::abs(power(neutral.signal)-.125)<1e-10,"Neutral optical path is not unity");
    p.optoColor=.85;auto coloured=optical(p);double thd=std::hypot(harmonic(coloured.signal,2),harmonic(coloured.signal,3))/harmonic(coloured.signal,1);need(thd>.03,"Opto colour lacks audible harmonics");
    p.optoMix=0;auto dry=optical(p);need(std::abs(power(dry.signal)-.125)<1e-10,"Parallel mix=0 is not original input");
    p.optoMix=1;p.optoBypass=1;auto bypass=optical(p);need(std::abs(power(bypass.signal)-.125)<1e-10,"Bypass retains coloration");
    p.optoBypass=0;p.optoColor=0;p.optoAmount=.8;p.optoRecovery=.5;
    auto shortBurst=optical(p,.5,4,.15),longBurst=optical(p,.5,7,3);
    double shortTail=shortBurst.gr[int(sr*(.15+.3))],longTail=longBurst.gr[int(sr*3.3)];need(longTail>shortTail+2,"Optical memory does not depend on program duration");
    p.optoRecovery=0;auto quick=optical(p,.5,7,3);p.optoRecovery=1;auto slow=optical(p,.5,7,3);need(slow.gr[int(sr*4)]>quick.gr[int(sr*4)]+1,"Recovery control is ineffective");
    std::cout<<"Wow p95="<<wowPitch<<" cents; Flutter p95="<<flutterPitch<<" cents; Age deep 10ms windows="<<deepDrops<<"/1100, min level="<<20*std::log10(worstGain)<<" dB\n";
    std::cout<<"Opto max steady reduction="<<compressed.gr.back()<<" dB; Colour 85% H2/H3 THD="<<100*thd<<"%; short/long burst tail="<<shortTail<<"/"<<longTail<<" dB\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
