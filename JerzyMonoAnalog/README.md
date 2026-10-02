# Jerzy Mono Analog — DSP v0.2

Monofoniczny syntezator analog-modeling VST3/Standalone oparty na JUCE.

## DSP
- 2 x band-limited VCO (PolyBLEP): sine / triangle / saw / square
- octave per VCO, detune VCO2, PWM
- sub oscillator: sine / square, -1 oktawa
- white noise
- niezależny wolny drift VCO
- nieliniowy, lekko asymetryczny mixer
- 24 dB/oct nonlinear ladder-style VCF z iteracyjną pętlą feedback
- rezonans do samowzbudzenia, filter drive, key tracking, bipolar filter envelope
- dwa analogowo zakrzywione ADSR
- LFO: sine / triangle / saw / square / sample&hold
- LFO -> pitch / cutoff / PWM
- note priority: Last / Low / High
- legato, retrigger
- glide: Always / Legato-only
- nieliniowy VCA
- analog-style output/preamp saturation
- DC blocking
- stały 4x wewnętrzny rate
- 4x -> 1x anti-aliasing przez dwa 63-tap windowed-sinc decimatory 2x

## JUCE
CMake automatycznie pobiera JUCE 9.0.3 przez FetchContent. Nie trzeba dodawać katalogu JUCE ręcznie.

## Build Windows / FL Studio
Wymagane: Visual Studio 2022 z workloadem Desktop development with C++ oraz CMake.

```bat
build_windows.bat
```

albo ręcznie:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

## GitHub Actions
Workflow `.github/workflows/jerzy-mono-analog-windows.yml` buduje Windows x64 VST3. Po poprawnym buildzie artefakt ma nazwę:

`Jerzy-Mono-Analog-Windows-VST3`

Po rozpakowaniu skopiuj `Jerzy Mono Analog.vst3` do:

`C:\Program Files\Common Files\VST3`

Następnie wykonaj rescan w FL Studio.

## Status
GUI jest na razie technicznym placeholderem. Ta gałąź koncentruje się na stabilnym buildzie i DSP.
