#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
class HostAutomatableFloat final : public juce::AudioParameterFloat
{
public:
    using juce::AudioParameterFloat::AudioParameterFloat;
    bool isAutomatable() const override { return true; }
};
class HostAutomatableBool final : public juce::AudioParameterBool
{
public:
    using juce::AudioParameterBool::AudioParameterBool;
    bool isAutomatable() const override { return true; }
};
class HostAutomatableChoice final : public juce::AudioParameterChoice
{
public:
    using juce::AudioParameterChoice::AudioParameterChoice;
    bool isAutomatable() const override { return true; }
};
}

JerzyFXBlockAudioProcessor::JerzyFXBlockAudioProcessor()
: AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),
  apvts(*this,nullptr,"PARAMS",createLayout()) {}

void JerzyFXBlockAudioProcessor::prepareToPlay(double sr,int bs){engine.prepare(sr,bs);}
bool JerzyFXBlockAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo() && l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
}
void JerzyFXBlockAudioProcessor::setEffectOrder(const std::array<int,6>& o){for(int i=0;i<6;++i)fxOrder[(size_t)i].store(o[(size_t)i]);}
std::array<int,6> JerzyFXBlockAudioProcessor::getEffectOrder() const
{
    std::array<int,6> o{}; for(int i=0;i<6;++i)o[(size_t)i]=fxOrder[(size_t)i].load(); return o;
}
static float delayMsFromDivision(int d,double bpm)
{
    static constexpr double q[]={4.0,2.0,1.0,0.5,0.25,0.125,2.0/3.0,1.0/3.0,1.0/6.0,1.5,0.75,0.375};
    return (float)(60000.0/juce::jmax(1.0,bpm)*q[juce::jlimit(0,11,d)]);
}
static float rotaryHzFromDivision(int d,double bpm)
{
    static constexpr double cpq[]={0.25,0.5,1.0,2.0,4.0,8.0,1.5,3.0,6.0,2.0/3.0,4.0/3.0,8.0/3.0};
    return (float)((bpm/60.0)*cpq[juce::jlimit(0,11,d)]);
}
void JerzyFXBlockAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)
{
    juce::ScopedNoDenormals n;
    auto v=[&](const char* id){return apvts.getRawParameterValue(id)->load();};
    double bpm=120.0;
    if(auto* ph=getPlayHead()) if(auto pos=ph->getPosition()) if(auto hostBpm=pos->getBpm()) bpm=*hostBpm;
    auto choiceIndex=[&](const char* id)
    {
        if(auto* p=dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(id))) return p->getIndex();
        return (int)v(id);
    };
    float delTime=v("delTime");
    if(v("delSync")>.5f) delTime=delayMsFromDivision(choiceIndex("delDivision"),bpm);
    float rotRate=v("rotRate");
    if(v("rotSync")>.5f) rotRate=rotaryHzFromDivision(choiceIndex("rotDivision"),bpm);
    const auto order=getEffectOrder();

    for(int s=0;s<b.getNumSamples();++s)
    {
        float l=b.getSample(0,s), r=b.getSample(1,s);
        for(int slot=0;slot<6;++slot)
        {
            switch(order[(size_t)slot])
            {
                case 0: if(v("revOn")>.5f) engine.processReverb(l,r,v("revSize"),v("revDamp"),v("revMix")); break;
                case 1: if(v("delOn")>.5f) engine.processDelay(l,r,delTime,v("delFb"),v("delMix")); break;
                case 2: if(v("choOn")>.5f) engine.processChorus(l,r,v("choRate"),v("choDepth"),v("choMix")); break;
                case 3: if(v("widOn")>.5f) engine.processWidth(l,r,v("width"),v("widMix")); break;
                case 4: if(v("rotOn")>.5f) engine.processRotary(l,r,rotRate,v("rotDepth"),v("rotMix")); break;
                case 5: if(v("shOn")>.5f) engine.processShimmer(l,r,v("shAmt"),v("shMix")); break;
            }
        }
        b.setSample(0,s,l); b.setSample(1,s,r);
    }
}
juce::AudioProcessorEditor* JerzyFXBlockAudioProcessor::createEditor(){return new JerzyFXBlockAudioProcessorEditor(*this);}
void JerzyFXBlockAudioProcessor::getStateInformation(juce::MemoryBlock& m)
{
    auto st=apvts.copyState(); auto o=getEffectOrder();
    juce::String ord; for(int i=0;i<6;++i){if(i)ord<<",";ord<<o[(size_t)i];}
    st.setProperty("fxOrder",ord,nullptr);
    std::unique_ptr<juce::XmlElement>x(st.createXml());copyXmlToBinary(*x,m);
}
void JerzyFXBlockAudioProcessor::setStateInformation(const void*d,int n)
{
    std::unique_ptr<juce::XmlElement>x(getXmlFromBinary(d,n));
    if(!x)return;
    auto st=juce::ValueTree::fromXml(*x);
    if(st.hasProperty("fxOrder"))
    {
        auto toks=juce::StringArray::fromTokens(st["fxOrder"].toString(),",","");
        if(toks.size()==6){std::array<int,6>o{};for(int i=0;i<6;++i)o[(size_t)i]=toks[i].getIntValue();setEffectOrder(o);}
        st.removeProperty("fxOrder",nullptr);
    }
    apvts.replaceState(st);
}
juce::AudioProcessorValueTreeState::ParameterLayout JerzyFXBlockAudioProcessor::createLayout()
{
    using P = HostAutomatableFloat; using B = HostAutomatableBool; using C = HostAutomatableChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto addFx=[&](const char* on){l.add(std::make_unique<B>(on,on,true));};
    addFx("revOn"); l.add(std::make_unique<P>("revSize","Reverb Size",0.f,1.f,.55f)); l.add(std::make_unique<P>("revDamp","Reverb Damp",0.f,1.f,.45f)); l.add(std::make_unique<P>("revMix","Reverb Mix",0.f,1.f,.25f));
    addFx("delOn"); l.add(std::make_unique<P>("delTime","Delay Time",1.f,2000.f,380.f)); l.add(std::make_unique<P>("delFb","Delay Feedback",0.f,.95f,.35f)); l.add(std::make_unique<P>("delMix","Delay Mix",0.f,1.f,.25f));
    l.add(std::make_unique<B>("delSync","Delay Host Sync",false));
    l.add(std::make_unique<C>("delDivision","Delay Division",juce::StringArray{"1/1","1/2","1/4","1/8","1/16","1/32","1/4T","1/8T","1/16T","1/4D","1/8D","1/16D"},3));
    addFx("choOn"); l.add(std::make_unique<P>("choRate","Chorus Rate",.05f,8.f,.7f)); l.add(std::make_unique<P>("choDepth","Chorus Depth",0.f,12.f,4.f)); l.add(std::make_unique<P>("choMix","Chorus Mix",0.f,1.f,.3f));
    addFx("widOn"); l.add(std::make_unique<P>("width","Stereo Width",0.f,1.f,.45f)); l.add(std::make_unique<P>("widMix","Stereo Mix",0.f,1.f,1.f));
    addFx("rotOn"); l.add(std::make_unique<P>("rotRate","Rotary Rate",.05f,8.f,.8f)); l.add(std::make_unique<P>("rotDepth","Rotary Depth",0.f,1.f,.6f)); l.add(std::make_unique<P>("rotMix","Rotary Mix",0.f,1.f,.45f));
    l.add(std::make_unique<B>("rotSync","Rotary Host Sync",false));
    l.add(std::make_unique<C>("rotDivision","Rotary Division",juce::StringArray{"1/1","1/2","1/4","1/8","1/16","1/32","1/4T","1/8T","1/16T","1/4D","1/8D","1/16D"},3));
    addFx("shOn"); l.add(std::make_unique<P>("shAmt","Shimmer Amount",0.f,1.f,.55f)); l.add(std::make_unique<P>("shMix","Shimmer Mix",0.f,1.f,.25f));
    return l;
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new JerzyFXBlockAudioProcessor();}
