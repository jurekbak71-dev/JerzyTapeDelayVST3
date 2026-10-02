# JERZY AUTO TUNE 1.0

Windows x64 VST3 vocal pitch correction plug-in for FL Studio.

## Interface and controls

The plug-in opens its own resizable analog-console interface. It contains:

- **Key** and **Scale** selectors.
- **Note Filter** buttons C through B. Enabled notes remain available as correction targets; disabled notes are excluded.
- **Speed** from 0 to 100 ms. Lower values retune faster.
- **Amount** and **Mix** controls.
- Output **Analog Compressor** with Threshold, Ratio, Makeup and bypass.
- Output **Analog Equalizer** with Low, Mid, High, Output and bypass.
- Input/output meters.

Compressor and EQ parameters are stored in the FL Studio project and are available for automation. The EQ and compressor are applied after pitch correction.

## Install in FL Studio

1. Close FL Studio.
2. Remove the older JERZY AUTO TUNE.vst3 from C:\Program Files\Common Files\VST3 if it is present.
3. Extract the new JERZY AUTO TUNE.vst3 folder from the ZIP into C:\Program Files\Common Files\VST3.
4. Open FL Studio and run Plugin Manager > Find installed plugins.
5. Load the new JERZY AUTO TUNE entry. This build has a new VST3 identifier and version 1.0.0.

## Build

This project uses JUCE 8.0.6 and Signalsmith Stretch. On Windows with CMake and Visual Studio, run:

    cmake -S JerzyAutoTune -B build/JerzyAutoTune -A x64
    cmake --build build/JerzyAutoTune --config Release --target JerzyAutoTune_VST3
