#include "PluginProcessor.h"
#include "PluginEditor.h"

JerzyMonoAnalogAudioProcessor::JerzyMonoAnalogAudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  apvts(*this, nullptr, "PARAMS", createLayout()) {}

void JerzyMonoAnalogAudioProcessor::prepareToPlay(double sr, int bs) { engine.prepare(sr, bs); }

bool JerzyMonoAnalogAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

static jerzy::BandLimitedOscillator::Wave waveFrom(float v)
{
    switch ((int) v) { case 0: return jerzy::BandLimitedOscillator::Wave::sine; case 1: return jerzy::BandLimitedOscillator::Wave::triangle; case 2: return jerzy::BandLimitedOscillator::Wave::saw; default: return jerzy::BandLimitedOscillator::Wave::square; }
}
static jerzy::AnalogLFO::Wave lfoWaveFrom(float v)
{
    switch ((int) v) { case 0: return jerzy::AnalogLFO::Wave::sine; case 1: return jerzy::AnalogLFO::Wave::triangle; case 2: return jerzy::AnalogLFO::Wave::saw; case 3: return jerzy::AnalogLFO::Wave::square; default: return jerzy::AnalogLFO::Wave::sampleHold; }
}

void JerzyMonoAnalogAudioProcessor::processBlock(juce::AudioBuffer<float>& b, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    b.clear();
    jerzy::MonoParameters p;
    p.osc1Wave = waveFrom(apvts.getRawParameterValue("osc1Wave")->load());
    p.osc2Wave = waveFrom(apvts.getRawParameterValue("osc2Wave")->load());
    p.subWave = waveFrom(apvts.getRawParameterValue("subWave")->load()) == jerzy::BandLimitedOscillator::Wave::sine ? jerzy::BandLimitedOscillator::Wave::sine : jerzy::BandLimitedOscillator::Wave::square;
    p.osc1Octave = (int) apvts.getRawParameterValue("osc1Oct")->load() - 2;
    p.osc2Octave = (int) apvts.getRawParameterValue("osc2Oct")->load() - 2;
    p.osc1Level = apvts.getRawParameterValue("osc1Level")->load();
    p.osc2Level = apvts.getRawParameterValue("osc2Level")->load();
    p.subLevel = apvts.getRawParameterValue("subLevel")->load();
    p.noiseLevel = apvts.getRawParameterValue("noiseLevel")->load();
    p.osc2DetuneCents = apvts.getRawParameterValue("detune")->load();
    p.pulseWidth = apvts.getRawParameterValue("pw")->load();
    p.mixerDrive = apvts.getRawParameterValue("mixDrive")->load();
    p.cutoffHz = apvts.getRawParameterValue("cutoff")->load();
    p.resonance = apvts.getRawParameterValue("resonance")->load();
    p.filterDrive = apvts.getRawParameterValue("filterDrive")->load();
    p.filterEnvOct = apvts.getRawParameterValue("filterEnv")->load();
    p.keyTrack = apvts.getRawParameterValue("keyTrack")->load();
    p.filterAttack = apvts.getRawParameterValue("fA")->load();
    p.filterDecay = apvts.getRawParameterValue("fD")->load();
    p.filterSustain = apvts.getRawParameterValue("fS")->load();
    p.filterRelease = apvts.getRawParameterValue("fR")->load();
    p.ampAttack = apvts.getRawParameterValue("aA")->load();
    p.ampDecay = apvts.getRawParameterValue("aD")->load();
    p.ampSustain = apvts.getRawParameterValue("aS")->load();
    p.ampRelease = apvts.getRawParameterValue("aR")->load();
    p.glideSeconds = apvts.getRawParameterValue("glide")->load();
    p.lfoWave = lfoWaveFrom(apvts.getRawParameterValue("lfoWave")->load());
    p.lfoRate = apvts.getRawParameterValue("lfoRate")->load();
    p.lfoPitchCents = apvts.getRawParameterValue("lfoPitch")->load();
    p.lfoFilterOct = apvts.getRawParameterValue("lfoFilter")->load();
    p.lfoPWM = apvts.getRawParameterValue("lfoPWM")->load();
    p.outputDrive = apvts.getRawParameterValue("outDrive")->load();
    p.master = apvts.getRawParameterValue("master")->load();
    p.analogDriftCents = apvts.getRawParameterValue("drift")->load();
    p.legato = apvts.getRawParameterValue("legato")->load() > 0.5f;
    p.retrigger = apvts.getRawParameterValue("retrigger")->load() > 0.5f;
    const int pr = (int) apvts.getRawParameterValue("priority")->load();
    p.priority = pr == 1 ? jerzy::NotePriority::low : (pr == 2 ? jerzy::NotePriority::high : jerzy::NotePriority::last);
    p.glideMode = apvts.getRawParameterValue("glideMode")->load() > 0.5f ? jerzy::GlideMode::legatoOnly : jerzy::GlideMode::always;
    engine.setParameters(p);

    auto it = midi.cbegin();
    juce::MidiMessageMetadata ev;
    bool has = it != midi.cend();
    if (has) ev = *it;
    for (int s = 0; s < b.getNumSamples(); ++s)
    {
        while (has && ev.samplePosition <= s)
        {
            auto m = ev.getMessage();
            if (m.isNoteOn()) engine.noteOn(m.getNoteNumber(), m.getFloatVelocity());
            else if (m.isNoteOff()) engine.noteOff(m.getNoteNumber());
            ++it; has = it != midi.cend(); if (has) ev = *it;
        }
        const float y = engine.processSample();
        for (int ch = 0; ch < b.getNumChannels(); ++ch) b.setSample(ch, s, y);
        const float ay = std::abs(y);
        const float old = outputMeter.load();
        outputMeter.store(ay > old ? ay : old * 0.9975f);
    }
}

juce::AudioProcessorEditor* JerzyMonoAnalogAudioProcessor::createEditor() { return new JerzyMonoAnalogAudioProcessorEditor(*this); }

void JerzyMonoAnalogAudioProcessor::getStateInformation(juce::MemoryBlock& mb)
{
    auto state = apvts.copyState(); std::unique_ptr<juce::XmlElement> xml(state.createXml()); copyXmlToBinary(*xml, mb);
}
void JerzyMonoAnalogAudioProcessor::setStateInformation(const void* d, int n)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(d, n)); if (xml) apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout JerzyMonoAnalogAudioProcessor::createLayout()
{
    using P = juce::AudioParameterFloat; using B = juce::AudioParameterBool; using C = juce::AudioParameterChoice;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add(std::make_unique<C>("osc1Wave","OSC1 Wave",juce::StringArray{"Sine","Triangle","Saw","Square"},2));
    l.add(std::make_unique<C>("osc2Wave","OSC2 Wave",juce::StringArray{"Sine","Triangle","Saw","Square"},2));
    l.add(std::make_unique<C>("subWave","Sub Wave",juce::StringArray{"Sine","Square"},1));
    l.add(std::make_unique<C>("osc1Oct","OSC1 Octave",juce::StringArray{"16'","8'","4'","2'","1'"},2));
    l.add(std::make_unique<C>("osc2Oct","OSC2 Octave",juce::StringArray{"16'","8'","4'","2'","1'"},2));
    l.add(std::make_unique<P>("osc1Level","OSC1 Level",0.0f,1.0f,0.75f));
    l.add(std::make_unique<P>("osc2Level","OSC2 Level",0.0f,1.0f,0.55f));
    l.add(std::make_unique<P>("subLevel","Sub Level",0.0f,1.0f,0.25f));
    l.add(std::make_unique<P>("noiseLevel","Noise Level",0.0f,1.0f,0.0f));
    l.add(std::make_unique<P>("detune","OSC2 Detune",-50.0f,50.0f,7.0f));
    l.add(std::make_unique<P>("pw","Pulse Width",0.05f,0.95f,0.5f));
    l.add(std::make_unique<P>("mixDrive","Mixer Drive",0.0f,1.0f,0.18f));
    l.add(std::make_unique<P>("cutoff","Cutoff",juce::NormalisableRange<float>(20.0f,20000.0f,0.0f,0.22f),1800.0f));
    l.add(std::make_unique<P>("resonance","Resonance",0.0f,1.15f,0.15f));
    l.add(std::make_unique<P>("filterDrive","Filter Drive",0.0f,1.0f,0.12f));
    l.add(std::make_unique<P>("filterEnv","Filter Env",-6.0f,6.0f,2.5f));
    l.add(std::make_unique<P>("keyTrack","Key Track",0.0f,1.0f,0.25f));
    auto sec=[](const char* id,const char* nm,float def,float max){return std::make_unique<P>(id,nm,juce::NormalisableRange<float>(0.0005f,max,0.0f,0.3f),def);};
    l.add(sec("fA","Filter Attack",0.002f,10.0f)); l.add(sec("fD","Filter Decay",0.22f,15.0f));
    l.add(std::make_unique<P>("fS","Filter Sustain",0.0f,1.0f,0.2f)); l.add(sec("fR","Filter Release",0.18f,20.0f));
    l.add(sec("aA","Amp Attack",0.005f,10.0f)); l.add(sec("aD","Amp Decay",0.18f,15.0f));
    l.add(std::make_unique<P>("aS","Amp Sustain",0.0f,1.0f,0.75f)); l.add(sec("aR","Amp Release",0.22f,20.0f));
    l.add(std::make_unique<P>("glide","Glide",juce::NormalisableRange<float>(0.0f,2.0f,0.0f,0.35f),0.0f));
    l.add(std::make_unique<C>("glideMode","Glide Mode",juce::StringArray{"Always","Legato"},1));
    l.add(std::make_unique<C>("priority","Note Priority",juce::StringArray{"Last","Low","High"},0));
    l.add(std::make_unique<C>("lfoWave","LFO Wave",juce::StringArray{"Sine","Triangle","Saw","Square","S&H"},0));
    l.add(std::make_unique<P>("lfoRate","LFO Rate",juce::NormalisableRange<float>(0.03f,30.0f,0.0f,0.25f),2.0f));
    l.add(std::make_unique<P>("lfoPitch","LFO Pitch",0.0f,100.0f,0.0f));
    l.add(std::make_unique<P>("lfoFilter","LFO Filter",0.0f,4.0f,0.0f));
    l.add(std::make_unique<P>("lfoPWM","LFO PWM",0.0f,1.0f,0.0f));
    l.add(std::make_unique<P>("outDrive","Output Drive",0.0f,1.0f,0.12f));
    l.add(std::make_unique<P>("master","Master",0.0f,1.0f,0.8f));
    l.add(std::make_unique<P>("drift","Analog Drift",0.0f,6.0f,2.0f));
    l.add(std::make_unique<B>("legato","Legato",true)); l.add(std::make_unique<B>("retrigger","Retrigger",false));
    return l;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new JerzyMonoAnalogAudioProcessor(); }
