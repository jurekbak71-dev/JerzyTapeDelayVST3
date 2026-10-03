#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace jerzyfx
{
inline float softClip(float x){ return std::tanh(x); }

class StereoDelay
{
public:
    void prepare(double sr, int maxBlock)
    {
        sampleRate=sr; juce::ignoreUnused(maxBlock);
        const int maxS=(int)(sr*4.0)+8;
        dl.resize(maxS); dr.resize(maxS); clear();
    }
    void clear(){ std::fill(dl.begin(),dl.end(),0.f); std::fill(dr.begin(),dr.end(),0.f); wp=0; }
    void process(float& l,float& r,float msL,float msR,float fb,float mix)
    {
        const int n=(int)dl.size();
        auto read=[&](const std::vector<float>& b,float ms){
            float ds=juce::jlimit(1.0f,(float)n-3.0f,(float)(sampleRate*ms*0.001));
            float pos=(float)wp-ds; while(pos<0)pos+=n;
            int i0=(int)pos, i1=(i0+1)%n; float f=pos-i0;
            return b[(size_t)i0]*(1.f-f)+b[(size_t)i1]*f;
        };
        float yl=read(dl,msL), yr=read(dr,msR);
        dl[(size_t)wp]=softClip(l+yr*fb);
        dr[(size_t)wp]=softClip(r+yl*fb);
        wp=(wp+1)%n;
        l=l*(1-mix)+yl*mix; r=r*(1-mix)+yr*mix;
    }
private:
    double sampleRate=44100; std::vector<float> dl,dr; int wp=0;
};

class ModDelay
{
public:
    void prepare(double sr){ sampleRate=sr; const int n=(int)(sr*0.08)+16; bl.resize(n); br.resize(n); clear(); }
    void clear(){std::fill(bl.begin(),bl.end(),0.f);std::fill(br.begin(),br.end(),0.f);wp=0;phase=0;}
    void process(float& l,float& r,float rate,float depthMs,float baseMs,float mix,float stereoPhase=0.25f)
    {
        bl[(size_t)wp]=l; br[(size_t)wp]=r;
        float wl=0.5f+0.5f*std::sin(juce::MathConstants<float>::twoPi*phase);
        float wr=0.5f+0.5f*std::sin(juce::MathConstants<float>::twoPi*(phase+stereoPhase));
        auto rd=[&](const std::vector<float>& b,float mod){
            float ds=(baseMs+depthMs*mod)*(float)sampleRate*0.001f;
            float pos=(float)wp-ds; const int n=(int)b.size(); while(pos<0)pos+=n;
            int i0=(int)pos, i1=(i0+1)%n; float f=pos-i0;
            return b[(size_t)i0]*(1-f)+b[(size_t)i1]*f;
        };
        float yl=rd(bl,wl), yr=rd(br,wr);
        l=l*(1-mix)+yl*mix; r=r*(1-mix)+yr*mix;
        wp=(wp+1)%(int)bl.size();
        phase += rate/(float)sampleRate; if(phase>=1)phase-=1;
    }
private:
    double sampleRate=44100; std::vector<float> bl,br; int wp=0; float phase=0;
};

class StereoExpander
{
public:
    void process(float& l,float& r,float width,float mix)
    {
        const float dryL=l,dryR=r;
        float m=(l+r)*0.5f, s=(l-r)*0.5f*(1.0f+width*2.0f);
        l=m+s; r=m-s;
        l=dryL*(1-mix)+l*mix; r=dryR*(1-mix)+r*mix;
    }
};

class Rotary
{
public:
    void prepare(double sr){sampleRate=sr; delay.prepare(sr);phase=0;}
    void process(float& l,float& r,float rate,float depth,float mix)
    {
        const float dryL=l,dryR=r;
        float pan=std::sin(juce::MathConstants<float>::twoPi*phase);
        float ampL=0.78f+0.22f*pan, ampR=0.78f-0.22f*pan;
        l*=ampL; r*=ampR;
        delay.process(l,r, rate*0.5f, depth*2.5f, 2.5f, depth*0.7f, 0.35f);
        phase+=rate/(float)sampleRate; if(phase>=1)phase-=1;
        l=dryL*(1-mix)+l*mix; r=dryR*(1-mix)+r*mix;
    }
private:
    double sampleRate=44100; float phase=0; ModDelay delay;
};

class PitchUpOctave
{
public:
    void prepare(double sr)
    {
        sampleRate=sr; size=(int)(sr*0.12)+16; bl.assign(size,0); br.assign(size,0); wp=0; ph=0;
    }
    void process(float inL,float inR,float& outL,float& outR)
    {
        bl[(size_t)wp]=inL; br[(size_t)wp]=inR;
        auto tap=[&](const std::vector<float>& b,float p){
            const float maxD=(float)sampleRate*0.055f;
            float d=maxD*(1.0f-p);
            float pos=(float)wp-d; while(pos<0)pos+=size;
            int i0=(int)pos, i1=(i0+1)%size; float f=pos-i0;
            return b[(size_t)i0]*(1-f)+b[(size_t)i1]*f;
        };
        float p1=ph, p2=std::fmod(ph+0.5f,1.0f);
        float w1=0.5f-0.5f*std::cos(juce::MathConstants<float>::twoPi*p1);
        float w2=0.5f-0.5f*std::cos(juce::MathConstants<float>::twoPi*p2);
        outL=(tap(bl,p1)*w1+tap(bl,p2)*w2)/(w1+w2+1e-6f);
        outR=(tap(br,p1)*w1+tap(br,p2)*w2)/(w1+w2+1e-6f);
        wp=(wp+1)%size;
        ph += 2.0f/(float)(sampleRate*0.055f); if(ph>=1)ph-=1;
    }
private:
    double sampleRate=44100; int size=0,wp=0; float ph=0; std::vector<float> bl,br;
};

class FXEngine
{
public:
    void prepare(double sr,int maxBlock)
    {
        sampleRate=sr;
        delay.prepare(sr,maxBlock);
        chorus.prepare(sr); rotary.prepare(sr); pitch.prepare(sr);
        reverb.setSampleRate(sr); shimmerVerb.setSampleRate(sr);
    }
    void process(float& l,float& r,
                 bool revOn,float revSize,float revDamp,float revMix,
                 bool delOn,float delMs,float delFb,float delMix,
                 bool choOn,float choRate,float choDepth,float choMix,
                 bool widOn,float width,float widMix,
                 bool rotOn,float rotRate,float rotDepth,float rotMix,
                 bool shOn,float shAmt,float shMix)
    {
        if(revOn)
        {
            juce::Reverb::Parameters p; p.roomSize=revSize; p.damping=revDamp; p.wetLevel=revMix; p.dryLevel=1-revMix; p.width=1;
            reverb.setParameters(p); reverb.processStereo(&l,&r,1);
        }
        if(delOn) delay.process(l,r,delMs,delMs*1.013f,delFb,delMix);
        if(choOn) chorus.process(l,r,choRate,choDepth,12.0f,choMix,0.23f);
        if(widOn) expander.process(l,r,width,widMix);
        if(rotOn) rotary.process(l,r,rotRate,rotDepth,rotMix);
        if(shOn)
        {
            float upL=0,upR=0; pitch.process(l,r,upL,upR);
            juce::Reverb::Parameters p; p.roomSize=0.88f; p.damping=0.25f; p.wetLevel=0.75f*shAmt; p.dryLevel=0; p.width=1;
            shimmerVerb.setParameters(p); shimmerVerb.processStereo(&upL,&upR,1);
            l=l*(1-shMix)+upL*shMix; r=r*(1-shMix)+upR*shMix;
        }
    }
private:
    double sampleRate=44100;
    juce::Reverb reverb,shimmerVerb;
    StereoDelay delay; ModDelay chorus; StereoExpander expander; Rotary rotary; PitchUpOctave pitch;
};
}
