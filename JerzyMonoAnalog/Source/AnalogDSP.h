#pragma once
#include <JuceHeader.h>
#include <array>
#include <random>
#include <cmath>
#include <vector>

namespace jerzy
{
static inline double saturateAsymmetric(double x)
{
    // Mild asymmetry gives a little even-harmonic content without hard clipping.
    const double shaped = x + 0.028 * x * x - 0.004 * x * x * x;
    return std::tanh(shaped);
}

class DCBlocker
{
public:
    void prepare(double fs, double cutoffHz = 7.0)
    {
        const double rc = 1.0 / (2.0 * juce::MathConstants<double>::pi * cutoffHz);
        const double dt = 1.0 / fs;
        r = rc / (rc + dt);
        reset();
    }
    double process(double x)
    {
        const double y = r * (z + x - x1);
        x1 = x; z = y;
        return y;
    }
    void reset() { x1 = z = 0.0; }
private:
    double r = 0.995, x1 = 0.0, z = 0.0;
};

class SmoothRandom
{
public:
    void prepare(double fs, double speedHz, uint32_t seed)
    {
        sampleRate = fs;
        speed = juce::jmax(0.001, speedHz);
        rng.seed(seed);
        current = target = randomValue();
        setNextTarget();
    }
    double next()
    {
        if (++count >= segmentSamples)
            setNextTarget();
        const double t = (double) count / (double) segmentSamples;
        const double s = t * t * (3.0 - 2.0 * t);
        return current + (target - current) * s;
    }
private:
    double randomValue() { return dist(rng); }
    void setNextTarget()
    {
        current = target;
        target = randomValue();
        const double wander = 0.7 + 0.6 * std::abs(randomValue());
        segmentSamples = (int) juce::jmax(16.0, sampleRate / speed * wander);
        count = 0;
    }
    double sampleRate = 44100.0, speed = 0.2, current = 0.0, target = 0.0;
    int count = 0, segmentSamples = 44100;
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist {-1.0, 1.0};
};

class BandLimitedOscillator
{
public:
    enum class Wave { sine, triangle, saw, square };

    void prepare(double fs, uint32_t seed)
    {
        sampleRate = fs;
        drift.prepare(fs, 0.17 + (seed & 7u) * 0.007, seed);
        reset();
    }
    void reset() { phase = 0.0; triState = 0.0; }
    void setWave(Wave w) { wave = w; }
    void setFrequency(double hz) { frequency = juce::jlimit(0.01, sampleRate * 0.45, hz); }
    void setPulseWidth(double p) { pw = juce::jlimit(0.05, 0.95, p); }
    void setDriftCents(double c) { driftCents = juce::jlimit(0.0, 8.0, c); }

    double process()
    {
        const double cents = drift.next() * driftCents;
        const double f = frequency * std::pow(2.0, cents / 1200.0);
        const double dt = juce::jmin(0.49, f / sampleRate);
        double y = 0.0;

        switch (wave)
        {
            case Wave::sine:
                y = std::sin(juce::MathConstants<double>::twoPi * phase);
                break;
            case Wave::saw:
                y = 2.0 * phase - 1.0;
                y -= polyBlep(phase, dt);
                break;
            case Wave::square:
                y = (phase < pw ? 1.0 : -1.0);
                y += polyBlep(phase, dt);
                y -= polyBlep(wrapped(phase - pw), dt);
                break;
            case Wave::triangle:
            {
                double sq = (phase < 0.5 ? 1.0 : -1.0);
                sq += polyBlep(phase, dt);
                sq -= polyBlep(wrapped(phase + 0.5), dt);
                triState += 4.0 * dt * sq;
                triState *= 0.99997;
                y = juce::jlimit(-1.15, 1.15, triState);
                break;
            }
        }

        phase += dt;
        if (phase >= 1.0) phase -= 1.0;
        return y;
    }

private:
    static double wrapped(double x) { while (x < 0.0) x += 1.0; while (x >= 1.0) x -= 1.0; return x; }
    static double polyBlep(double t, double dt)
    {
        if (dt <= 0.0) return 0.0;
        if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }

    double sampleRate = 44100.0, phase = 0.0, frequency = 110.0, pw = 0.5;
    double triState = 0.0, driftCents = 2.0;
    Wave wave = Wave::saw;
    SmoothRandom drift;
};

class AnalogADSR
{
public:
    void prepare(double fs) { sampleRate = fs; reset(); }
    void reset() { stage = Stage::idle; value = 0.0; }
    void set(double a, double d, double s, double r)
    {
        attack = juce::jmax(0.0005, a);
        decay = juce::jmax(0.002, d);
        sustain = juce::jlimit(0.0, 1.0, s);
        release = juce::jmax(0.002, r);
    }
    void noteOn() { stage = Stage::attack; }
    void noteOff() { if (stage != Stage::idle) stage = Stage::release; }
    double process()
    {
        switch (stage)
        {
            case Stage::idle: value = 0.0; break;
            case Stage::attack:
                value += (1.0 - value) * coeff(attack, 6.2);
                if (value > 0.9994) { value = 1.0; stage = Stage::decay; }
                break;
            case Stage::decay:
                value += (sustain - value) * coeff(decay, 5.2);
                if (std::abs(value - sustain) < 1.0e-5) { value = sustain; stage = Stage::sustain; }
                break;
            case Stage::sustain:
                value = sustain;
                break;
            case Stage::release:
                value += (0.0 - value) * coeff(release, 5.2);
                if (value < 1.0e-6) { value = 0.0; stage = Stage::idle; }
                break;
        }
        return value;
    }
private:
    double coeff(double seconds, double curve) const { return 1.0 - std::exp(-curve / (seconds * sampleRate)); }
    enum class Stage { idle, attack, decay, sustain, release } stage = Stage::idle;
    double sampleRate = 44100.0, attack = 0.01, decay = 0.2, sustain = 0.7, release = 0.3, value = 0.0;
};

class AnalogLFO
{
public:
    enum class Wave { sine, triangle, saw, square, sampleHold };
    void prepare(double fs, uint32_t seed)
    {
        sampleRate = fs; rng.seed(seed); reset();
    }
    void reset() { phase = 0.0; sh = random(); }
    void set(double hz, Wave w) { freq = juce::jlimit(0.01, 40.0, hz); wave = w; }
    double process()
    {
        double y = 0.0;
        switch (wave)
        {
            case Wave::sine: y = std::sin(juce::MathConstants<double>::twoPi * phase); break;
            case Wave::triangle: y = 1.0 - 4.0 * std::abs(phase - 0.5); break;
            case Wave::saw: y = 2.0 * phase - 1.0; break;
            case Wave::square: y = phase < 0.5 ? 1.0 : -1.0; break;
            case Wave::sampleHold: y = sh; break;
        }
        const double old = phase;
        phase += freq / sampleRate;
        if (phase >= 1.0) phase -= 1.0;
        if (wave == Wave::sampleHold && phase < old) sh = random();
        return y;
    }
private:
    double random() { return dist(rng); }
    double sampleRate = 44100.0, freq = 2.0, phase = 0.0, sh = 0.0;
    Wave wave = Wave::sine;
    std::mt19937 rng;
    std::uniform_real_distribution<double> dist {-1.0, 1.0};
};

class HalfBandDecimator2x
{
public:
    static constexpr int taps = 63;
    void prepare()
    {
        constexpr double fc = 0.235;
        constexpr int M = taps - 1;
        double sum = 0.0;
        for (int n = 0; n < taps; ++n)
        {
            const double m = (double)n - 0.5 * M;
            const double sinc = (std::abs(m) < 1.0e-12)
                              ? 2.0 * fc
                              : std::sin(2.0 * juce::MathConstants<double>::pi * fc * m)
                                / (juce::MathConstants<double>::pi * m);
            const double w = 0.42 - 0.5 * std::cos(2.0 * juce::MathConstants<double>::pi * n / M)
                                  + 0.08 * std::cos(4.0 * juce::MathConstants<double>::pi * n / M);
            coeff[(size_t)n] = sinc * w;
            sum += coeff[(size_t)n];
        }
        for (auto& c : coeff) c /= sum;
        reset();
    }
    void reset() { history.fill(0.0); write = 0; phase = 0; }
    bool process(double x, double& out)
    {
        history[(size_t)write] = x;
        write = (write + 1) % taps;
        phase ^= 1;
        if (phase != 0) return false;

        double y = 0.0;
        int idx = write;
        for (int i = 0; i < taps; ++i)
        {
            idx = (idx - 1 + taps) % taps;
            y += coeff[(size_t)i] * history[(size_t)idx];
        }
        out = y;
        return true;
    }
private:
    std::array<double, taps> coeff {}, history {};
    int write = 0, phase = 0;
};

class NonlinearLadder
{
public:
    void prepare(double fs)
    {
        sampleRate = fs;
        cutoffDrift.prepare(fs, 0.095, 0x3141592u);
        reset();
    }
    void reset()
    {
        state.fill(0.0);
        dc.prepare(sampleRate, 6.0);
    }
    void setParams(double cutoffHz, double resonance01, double drive01, double keyTrack01, double note)
    {
        baseCutoff = cutoffHz;
        resonance = juce::jlimit(0.0, 1.12, resonance01);
        drive = 1.0 + 8.0 * juce::jlimit(0.0, 1.0, drive01);
        keyTrack = juce::jlimit(0.0, 1.0, keyTrack01);
        midiNote = note;
    }

    double process(double x, double envAmountOctaves)
    {
        const double keyOct = (midiNote - 60.0) / 12.0 * keyTrack;
        const double driftMul = 1.0 + cutoffDrift.next() * 0.0025;
        double fc = baseCutoff * std::pow(2.0, keyOct + envAmountOctaves) * driftMul;
        fc = juce::jlimit(8.0, sampleRate * 0.42, fc);

        const double g = std::tan(juce::MathConstants<double>::pi * fc / sampleRate);
        const double G = g / (1.0 + g);
        const double k = 4.0 * resonance;
        const double input = saturateAsymmetric(x * drive) / std::sqrt(drive);

        double feedback = state[3];
        std::array<double, 4> solved = state;
        for (int iter = 0; iter < 3; ++iter)
        {
            auto tmp = state;
            double u = input - k * feedback;
            for (int s = 0; s < 4; ++s)
            {
                const double inN = std::tanh(u);
                const double stN = std::tanh(tmp[(size_t)s]);
                const double v = (inN - stN) * G;
                const double y = tmp[(size_t)s] + v;
                tmp[(size_t)s] = y + v;
                u = y;
            }
            solved = tmp;
            feedback = solved[3];
        }
        state = solved;
        return dc.process(state[3]);
    }
private:
    double sampleRate = 44100.0, baseCutoff = 1200.0, resonance = 0.0, drive = 1.0, keyTrack = 0.0, midiNote = 60.0;
    std::array<double, 4> state {};
    SmoothRandom cutoffDrift;
    DCBlocker dc;
};

enum class NotePriority { last, low, high };
enum class GlideMode { always, legatoOnly };

struct MonoParameters
{
    BandLimitedOscillator::Wave osc1Wave = BandLimitedOscillator::Wave::saw;
    BandLimitedOscillator::Wave osc2Wave = BandLimitedOscillator::Wave::saw;
    BandLimitedOscillator::Wave subWave = BandLimitedOscillator::Wave::square;
    AnalogLFO::Wave lfoWave = AnalogLFO::Wave::sine;
    int osc1Octave = 0, osc2Octave = 0;
    double osc1Level = 0.75, osc2Level = 0.55, subLevel = 0.25, noiseLevel = 0.0;
    double osc2DetuneCents = 7.0, pulseWidth = 0.5;
    double mixerDrive = 0.18;
    double cutoffHz = 1800.0, resonance = 0.15, filterDrive = 0.12, filterEnvOct = 2.5, keyTrack = 0.25;
    double ampAttack = 0.005, ampDecay = 0.18, ampSustain = 0.75, ampRelease = 0.22;
    double filterAttack = 0.002, filterDecay = 0.22, filterSustain = 0.2, filterRelease = 0.18;
    double glideSeconds = 0.0;
    double lfoRate = 2.0, lfoPitchCents = 0.0, lfoFilterOct = 0.0, lfoPWM = 0.0;
    double outputDrive = 0.12, master = 0.8, analogDriftCents = 2.0;
    NotePriority priority = NotePriority::last;
    GlideMode glideMode = GlideMode::legatoOnly;
    bool legato = true, retrigger = false;
};

class MonoAnalogEngine
{
public:
    void prepare(double fs, int maximumBlockSize)
    {
        juce::ignoreUnused(maximumBlockSize);
        hostRate = fs;
        osFactor = 4;
        sampleRate = fs * osFactor;
        osc1.prepare(sampleRate, 0x123456u);
        osc2.prepare(sampleRate, 0xabcdefu);
        sub.prepare(sampleRate, 0x77aa55u);
        lfo.prepare(sampleRate, 0x818181u);
        filter.prepare(sampleRate);
        ampEnv.prepare(sampleRate);
        filterEnv.prepare(sampleRate);
        outputDC.prepare(sampleRate);
        decimA.prepare();
        decimB.prepare();
        noiseRng.seed(0xdeadbeefu);
        reset();
    }

    void reset()
    {
        osc1.reset(); osc2.reset(); sub.reset(); lfo.reset(); filter.reset(); ampEnv.reset(); filterEnv.reset(); outputDC.reset();
        decimA.reset(); decimB.reset();
        currentNote = -1; targetMidi = currentMidi = 60.0; heldNotes.clear();
        lastOutput = 0.0;
    }

    void setParameters(const MonoParameters& p) { params = p; }

    void noteOn(int note, float velocity)
    {
        const bool hadHeldNotes = !heldNotes.isEmpty();
        heldNotes.removeAllInstancesOf(note);
        heldNotes.add(note);
        velocityGain = juce::jlimit(0.0, 1.0, (double)velocity);
        selectPriorityNote();

        const bool retrig = !hadHeldNotes || !params.legato || params.retrigger;
        if (retrig) { ampEnv.noteOn(); filterEnv.noteOn(); }

        const bool shouldGlide = params.glideSeconds > 0.0
                              && (params.glideMode == GlideMode::always || hadHeldNotes);
        if (!shouldGlide) currentMidi = targetMidi;
    }

    void noteOff(int note)
    {
        heldNotes.removeAllInstancesOf(note);
        if (heldNotes.isEmpty())
        {
            currentNote = -1;
            ampEnv.noteOff();
            filterEnv.noteOff();
            return;
        }

        selectPriorityNote();
        if (!params.legato && params.retrigger)
        {
            ampEnv.noteOn();
            filterEnv.noteOn();
        }
    }

    float processSample()
    {
        double aOut = 0.0, bOut = lastOutput;
        for (int i = 0; i < osFactor; ++i)
        {
            const double hi = processOversampled();
            if (decimA.process(hi, aOut))
            {
                double candidate = 0.0;
                if (decimB.process(aOut, candidate))
                {
                    bOut = candidate;
                    lastOutput = candidate;
                }
            }
        }
        return (float)juce::jlimit(-1.0, 1.0, bOut);
    }

private:
    void selectPriorityNote()
    {
        if (heldNotes.isEmpty()) return;
        int chosen = heldNotes.getLast();
        if (params.priority == NotePriority::low)
            chosen = *std::min_element(heldNotes.begin(), heldNotes.end());
        else if (params.priority == NotePriority::high)
            chosen = *std::max_element(heldNotes.begin(), heldNotes.end());
        currentNote = chosen;
        targetMidi = (double)chosen;
    }

    double processOversampled()
    {
        if (params.glideSeconds > 0.0)
        {
            const double a = 1.0 - std::exp(-1.0 / (params.glideSeconds * sampleRate));
            currentMidi += (targetMidi - currentMidi) * a;
        }
        else currentMidi = targetMidi;

        lfo.set(params.lfoRate, params.lfoWave);
        const double l = lfo.process();
        const double pitchMod = l * params.lfoPitchCents / 100.0;
        const double baseHz = 440.0 * std::pow(2.0, (currentMidi + pitchMod - 69.0) / 12.0);

        osc1.setWave(params.osc1Wave); osc2.setWave(params.osc2Wave); sub.setWave(params.subWave);
        const double modPW = juce::jlimit(0.05, 0.95, params.pulseWidth + l * params.lfoPWM * 0.45);
        osc1.setPulseWidth(modPW); osc2.setPulseWidth(modPW);
        osc1.setDriftCents(params.analogDriftCents);
        osc2.setDriftCents(params.analogDriftCents * 1.13);
        sub.setDriftCents(params.analogDriftCents * 0.12);
        osc1.setFrequency(baseHz * std::pow(2.0, params.osc1Octave));
        osc2.setFrequency(baseHz * std::pow(2.0, params.osc2Octave + params.osc2DetuneCents / 1200.0));
        sub.setFrequency(baseHz * 0.5);

        const double n = noiseDist(noiseRng);
        double mix = params.osc1Level * osc1.process()
                   + params.osc2Level * osc2.process()
                   + params.subLevel  * sub.process()
                   + params.noiseLevel * n;

        const double mixGain = 1.0 + 5.5 * params.mixerDrive;
        mix = saturateAsymmetric(mix * mixGain) / std::sqrt(mixGain);

        filterEnv.set(params.filterAttack, params.filterDecay, params.filterSustain, params.filterRelease);
        ampEnv.set(params.ampAttack, params.ampDecay, params.ampSustain, params.ampRelease);
        const double fe = filterEnv.process();
        const double ae = ampEnv.process();

        filter.setParams(params.cutoffHz, params.resonance, params.filterDrive, params.keyTrack, currentMidi);
        double y = filter.process(mix, fe * params.filterEnvOct + l * params.lfoFilterOct);

        const double vca = y * ae * velocityGain;
        y = saturateAsymmetric(vca * 1.22) / 1.10;

        const double outGain = 1.0 + 8.0 * params.outputDrive;
        y = saturateAsymmetric(y * outGain) / std::sqrt(outGain);
        y = outputDC.process(y) * params.master;
        return y;
    }

    double hostRate = 44100.0, sampleRate = 176400.0;
    int osFactor = 4;
    MonoParameters params;
    BandLimitedOscillator osc1, osc2, sub;
    AnalogLFO lfo;
    NonlinearLadder filter;
    AnalogADSR ampEnv, filterEnv;
    DCBlocker outputDC;
    HalfBandDecimator2x decimA, decimB;
    juce::Array<int> heldNotes;
    int currentNote = -1;
    double targetMidi = 60.0, currentMidi = 60.0, velocityGain = 1.0, lastOutput = 0.0;
    std::mt19937 noiseRng;
    std::uniform_real_distribution<double> noiseDist {-1.0, 1.0};
};

} // namespace jerzy
