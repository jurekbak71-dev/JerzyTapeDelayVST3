#include "PluginProcessor.h"
#include "PluginEditor.h"

JerzyFXBlockAudioProcessor::JerzyFXBlockAudioProcessor()
: AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),
  apvts(*this,nullptr,"PARAMS",createLayout()) {}

void JerzyFXBlockAudioProcessor::prepareToPlay(double sr,int bs){engine.prepare(sr,bs);}
bool JerzyFXBlockAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo() && l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();
}
void JerzyFXBlockAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)
{
    juce::ScopedNoDenormals n;
    auto v=[&](const char* id){return apvts.getRawParameterValue(id)->load();};
    for(int s=0;s<b.getNumSamples();++s)
    {
        float l=b.getSample(0,s), r=b.getSample(1,s);
        engine.process(l,r,
            v("revOn")>.5f,v("revSize"),v("revDamp"),v("revMix"),
            v("delOn")>.5f,v("delTime"),v("delFb"),v("delMix"),
            v("choOn")>.5f,v("choRate"),v("choDepth"),v("choMix"),
            v("widOn")>.5f,v("width"),v("widMix"),
            v("rotOn")>.5f,v("rotRate"),v("rotDepth"),v("rotMix"),
            v("shOn")>.5f,v("shAmt"),v("shMix"));
        b.setSample(0,s,l); b.setSample(1,s,r);
    }
}
juce::AudioProcessorEditor* JerzyFXBlockAudioProcessor::createEditor(){return new JerzyFXBlockAudioProcessorEditor(*this);}
void JerzyFXBlockAudioProcessor::getStateInformation(juce::MemoryBlock& m){auto st=apvts.copyState();std::unique_ptr<juce::XmlElement>x(st.createXml());copyXmlToBinary(*x,m);}
void JerzyFXBlockAudioProcessor::setStateInformation(const void*d,int n){std::unique_ptr<juce::XmlElement>x(getXmlFromBinary(d,n));if(x)apvts.replaceState(juce::ValueTree::fromXml(*x));}

juce::AudioProcessorValueTreeState::ParameterLayout JerzyFXBlockAudioProcessor::createLayout()
{
    using P=juce::AudioParameterFloat; using B=juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    auto addFx=[&](const char* on){l.add(std::make_unique<B>(on,on,true));};
    addFx("revOn"); l.add(std::make_unique<P>("revSize","Reverb Size",0.f,1.f,.55f)); l.add(std::make_unique<P>("revDamp","Reverb Damp",0.f,1.f,.45f)); l.add(std::make_unique<P>("revMix","Reverb Mix",0.f,1.f,.25f));
    addFx("delOn"); l.add(std::make_unique<P>("delTime","Delay Time",1.f,2000.f,380.f)); l.add(std::make_unique<P>("delFb","Delay Feedback",0.f,.95f,.35f)); l.add(std::make_unique<P>("delMix","Delay Mix",0.f,1.f,.25f));
    addFx("choOn"); l.add(std::make_unique<P>("choRate","Chorus Rate",.05f,8.f,.7f)); l.add(std::make_unique<P>("choDepth","Chorus Depth",0.f,12.f,4.f)); l.add(std::make_unique<P>("choMix","Chorus Mix",0.f,1.f,.3f));
    addFx("widOn"); l.add(std::make_unique<P>("width","Stereo Width",0.f,1.f,.45f)); l.add(std::make_unique<P>("widMix","Stereo Mix",0.f,1.f,1.f));
    addFx("rotOn"); l.add(std::make_unique<P>("rotRate","Rotary Rate",.05f,8.f,.8f)); l.add(std::make_unique<P>("rotDepth","Rotary Depth",0.f,1.f,.6f)); l.add(std::make_unique<P>("rotMix","Rotary Mix",0.f,1.f,.45f));
    addFx("shOn"); l.add(std::make_unique<P>("shAmt","Shimmer Amount",0.f,1.f,.55f)); l.add(std::make_unique<P>("shMix","Shimmer Mix",0.f,1.f,.25f));
    return l;
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new JerzyFXBlockAudioProcessor();}
