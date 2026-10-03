#include "PluginProcessor.h"
#include "PluginEditor.h"

JerzyMonoAnalogAudioProcessor::JerzyMonoAnalogAudioProcessor()
: AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
  apvts(*this, nullptr, "PARAMS", createLayout()) {}

void JerzyMonoAnalogAudioProcessor::prepareToPlay(double sr, int bs)
{
    currentSampleRate = sr;
    engine.prepare(sr, bs);
    resetArpState();
}

void JerzyMonoAnalogAudioProcessor::resetArpState()
{
    arpSamplesToNext = 0.0;
    arpStep = 0;
    arpCurrentNote = -1;
    arpUpDownPos = 0;
    arpHeldNotes.clear();
    arpLatchedNotes.clear();
    physicalHeldNotes.clear();
}

int JerzyMonoAnalogAudioProcessor::getChoiceIndex(const char* id) const
{
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(id)))
        return p->getIndex();
    return (int) apvts.getRawParameterValue(id)->load();
}

bool JerzyMonoAnalogAudioProcessor::arpRhythmGate(int rhythm, int step) const
{
    switch (rhythm)
    {
        case 1: return (step % 2) == 0;                           // every 2
        case 2: { static constexpr int m[8]={1,0,1,1,0,1,0,1}; return m[step & 7] != 0; } // 3-3-2 feel
        case 3: { static constexpr int m[8]={1,1,0,1,0,1,1,0}; return m[step & 7] != 0; } // syncopated
        default:return true;
    }
}

int JerzyMonoAnalogAudioProcessor::chooseArpNote(int pattern, int step)
{
    auto notes = arpLatchedNotes;
    if (notes.isEmpty()) return -1;
    std::sort(notes.begin(), notes.end());
    const int n = notes.size();

    switch (pattern)
    {
        case 1: return notes[(n - 1 - (step % n) + n) % n]; // down
        case 2:
        {
            if (n == 1) return notes[0];
            const int span = n * 2 - 2;
            const int p = step % span;
            return notes[p < n ? p : span - p];
        }
        case 3:
        {
            std::uniform_int_distribution<int> d(0, n - 1);
            return notes[d(arpRng)];
        }
        case 4: return notes[step % n]; // as played fallback
        default:return notes[step % n]; // up
    }
}

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

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto hostBpm = pos->getBpm())
                bpm = *hostBpm;

    if (apvts.getRawParameterValue("lfoSync")->load() > 0.5f)
    {
        static constexpr double cyclesPerQuarter[] =
        { 0.25,0.5,1.0,2.0,4.0,8.0,1.5,3.0,6.0,2.0/3.0,4.0/3.0,8.0/3.0 };
        const int idx = juce::jlimit(0,11,getChoiceIndex("lfoDivision"));
        p.lfoRate = (bpm / 60.0) * cyclesPerQuarter[idx];
    }

    p.lfoPitchCents = apvts.getRawParameterValue("lfoPitch")->load();
    p.lfoFilterOct = apvts.getRawParameterValue("lfoFilter")->load();
    p.lfoPWM = apvts.getRawParameterValue("lfoPWM")->load();
    p.lfoAmp = apvts.getRawParameterValue("lfoAmp")->load();
    p.lfoFadeSeconds = apvts.getRawParameterValue("lfoFade")->load();
    p.outputDrive = apvts.getRawParameterValue("outDrive")->load();
    p.master = apvts.getRawParameterValue("master")->load();
    p.analogDriftCents = apvts.getRawParameterValue("drift")->load();
    p.legato = apvts.getRawParameterValue("legato")->load() > 0.5f;
    p.retrigger = apvts.getRawParameterValue("retrigger")->load() > 0.5f;
    const int pr = getChoiceIndex("priority");
    p.priority = pr == 1 ? jerzy::NotePriority::low : (pr == 2 ? jerzy::NotePriority::high : jerzy::NotePriority::last);
    p.glideMode = getChoiceIndex("glideMode") > 0 ? jerzy::GlideMode::legatoOnly : jerzy::GlideMode::always;
    engine.setParameters(p);

    const bool arpOn = apvts.getRawParameterValue("arpOn")->load() > 0.5f;
    const bool arpLatch = apvts.getRawParameterValue("arpLatch")->load() > 0.5f;
    const bool arpRetrig = apvts.getRawParameterValue("arpRetrigger")->load() > 0.5f;
    const int arpPattern = getChoiceIndex("arpPattern");
    const int arpRhythm = getChoiceIndex("arpRhythm");
    const int arpDiv = getChoiceIndex("arpDivision");
    const int arpOctaves = 1 + getChoiceIndex("arpOctaves");
    const float arpGate = apvts.getRawParameterValue("arpGate")->load();

    static constexpr double quarterMult[] =
    { 4.0,2.0,1.0,0.5,0.25,0.125,2.0/3.0,1.0/3.0,1.0/6.0,1.5,0.75,0.375 };
    const double stepSamples = currentSampleRate * (60.0 / juce::jmax(1.0,bpm)) * quarterMult[juce::jlimit(0,11,arpDiv)];
    const double gateSamples = stepSamples * juce::jlimit(0.05f,0.98f,arpGate);

    auto midiIt = midi.cbegin();
    bool hasMidi = midiIt != midi.cend();
    juce::MidiMessageMetadata ev;
    if (hasMidi) ev = *midiIt;

    for (int s = 0; s < b.getNumSamples(); ++s)
    {
        while (hasMidi && ev.samplePosition <= s)
        {
            const auto m = ev.getMessage();
            if (m.isNoteOn())
            {
                const int note = m.getNoteNumber();
                if (!physicalHeldNotes.contains(note))
                    physicalHeldNotes.add(note);

                if (arpOn)
                {
                    if (arpLatch && physicalHeldNotes.size() == 1)
                        arpLatchedNotes.clear();

                    if (!arpHeldNotes.contains(note)) arpHeldNotes.add(note);
                    if (!arpLatchedNotes.contains(note)) arpLatchedNotes.add(note);
                    if (arpRetrig) { arpStep = 0; arpSamplesToNext = 0.0; }
                }
                else engine.noteOn(note, m.getFloatVelocity());
            }
            else if (m.isNoteOff())
            {
                const int note = m.getNoteNumber();
                physicalHeldNotes.removeAllInstancesOf(note);

                if (arpOn)
                {
                    arpHeldNotes.removeAllInstancesOf(note);
                    if (!arpLatch)
                        arpLatchedNotes.removeAllInstancesOf(note);
                }
                else engine.noteOff(note);
            }

            ++midiIt;
            hasMidi = midiIt != midi.cend();
            if (hasMidi) ev = *midiIt;
        }

        if (arpOn)
        {
            if (arpLatchedNotes.isEmpty())
            {
                if (arpCurrentNote >= 0) { engine.noteOff(arpCurrentNote); arpCurrentNote = -1; }
            }
            else
            {
                if (arpSamplesToNext <= 0.0)
                {
                    if (arpCurrentNote >= 0) { engine.noteOff(arpCurrentNote); arpCurrentNote = -1; }

                    if (arpRhythmGate(arpRhythm, arpStep))
                    {
                        const int base = chooseArpNote(arpPattern, arpStep);
                        if (base >= 0)
                        {
                            const int octave = (arpStep / juce::jmax(1,arpLatchedNotes.size())) % arpOctaves;
                            arpCurrentNote = juce::jlimit(0,127,base + octave * 12);
                            engine.noteOn(arpCurrentNote, 0.9f);
                        }
                    }
                    ++arpStep;
                    arpSamplesToNext += stepSamples;
                }

                if (arpCurrentNote >= 0 && arpSamplesToNext <= (stepSamples - gateSamples))
                {
                    engine.noteOff(arpCurrentNote);
                    arpCurrentNote = -1;
                }

                arpSamplesToNext -= 1.0;
            }
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
    l.add(std::make_unique<B>("lfoSync","LFO Tempo Sync",false));
    l.add(std::make_unique<C>("lfoDivision","LFO Division",
        juce::StringArray{"1/1","1/2","1/4","1/8","1/16","1/32","1/4T","1/8T","1/16T","1/4D","1/8D","1/16D"},3));
    l.add(std::make_unique<P>("lfoRate","LFO Rate",juce::NormalisableRange<float>(0.03f,30.0f,0.0f,0.25f),2.0f));
    l.add(std::make_unique<P>("lfoPitch","LFO Pitch",0.0f,100.0f,0.0f));
    l.add(std::make_unique<P>("lfoFilter","LFO Filter",0.0f,4.0f,0.0f));
    l.add(std::make_unique<P>("lfoPWM","LFO PWM",0.0f,1.0f,0.0f));
    l.add(std::make_unique<P>("lfoAmp","LFO Amp Mod",0.0f,1.0f,0.0f));
    l.add(std::make_unique<P>("lfoFade","LFO Fade In",juce::NormalisableRange<float>(0.0f,5.0f,0.0f,0.35f),0.0f));
    l.add(std::make_unique<P>("outDrive","Output Drive",0.0f,1.0f,0.12f));
    l.add(std::make_unique<P>("master","Master",0.0f,1.0f,0.8f));
    l.add(std::make_unique<P>("drift","Analog Drift",0.0f,6.0f,2.0f));
    l.add(std::make_unique<B>("legato","Legato",true)); l.add(std::make_unique<B>("retrigger","Retrigger",false));
    l.add(std::make_unique<B>("arpOn","Arpeggiator On",false));
    l.add(std::make_unique<C>("arpDivision","Arp Division",juce::StringArray{"1/1","1/2","1/4","1/8","1/16","1/32","1/4T","1/8T","1/16T","1/4D","1/8D","1/16D"},4));
    l.add(std::make_unique<C>("arpPattern","Arp Pattern",juce::StringArray{"Up","Down","UpDown","Random","As Played"},0));
    l.add(std::make_unique<C>("arpRhythm","Arp Rhythm",juce::StringArray{"Straight","Every 2","3-3-2","Syncopated"},0));
    l.add(std::make_unique<C>("arpOctaves","Arp Octaves",juce::StringArray{"1","2","3","4"},0));
    l.add(std::make_unique<P>("arpGate","Arp Gate",0.05f,0.98f,0.72f));
    l.add(std::make_unique<B>("arpLatch","Arp Latch",false));
    l.add(std::make_unique<B>("arpRetrigger","Arp Retrigger",true));
    return l;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new JerzyMonoAnalogAudioProcessor(); }
