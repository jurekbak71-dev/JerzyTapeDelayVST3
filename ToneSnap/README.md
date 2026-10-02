# JERZY AUTO TUNE — VST3

Efekt do korekcji monofonicznego wokalu. Wybierasz tonację i tryb skali, a wtyczka wykrywa wysokość dźwięku i przesuwa ją w stronę najbliższej dozwolonej nuty. GUI ma styl starej analogowej konsolety, z miernikiem VU oraz sekcją kompresora i korektora na wyjściu.

## Parametry

- **Key** — tonacja od C do B.
- **Scale** — chromatyczna (najbliższy półton), durowa lub molowa.
- **Speed** — czas reakcji korekcji w milisekundach: 0 ms daje najszybsze, wyraźnie słyszalne przejścia; większe wartości łagodzą korekcję.
- **Amount** — siła korekcji.
- **Mix** — proporcja sygnału przetworzonego.
- **Compressor** — włącznik oraz Threshold, Ratio i Makeup dla wyjściowej kompresji z miękkim kolanem.
- **Equalizer** — włącznik oraz półki Low (120 Hz), środek Mid (1,2 kHz), półka High (8 kHz) i Output (poziom wyjściowy). Pasmami EQ steruje się w zakresie ±12 dB.

Korekcja wysokości korzysta z Signalsmith Stretch (licencja MIT). Wykrywanie tonu używa pełnej, jednopróbkowej siatki opóźnień i interpolacji ułamkowej okresu, żeby ograniczyć skoki decyzji między nutami. Efekt zgłasza stałą latencję do hosta.

## Budowanie VST3 w Windows

Potrzebne są Visual Studio 2022 z C++ Desktop Development, CMake 3.24+ oraz dostęp do pobrania JUCE 8.0.6 i Signalsmith Stretch podczas konfiguracji.

W PowerShellu, w katalogu `ToneSnap`:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Wtyczka pojawi się w `build/JerzyAutoTune/JerzyAutoTune_artefacts/Release/VST3/JERZY AUTO TUNE.vst3`. Skopiuj ją do standardowego folderu VST3 (`C:\Program Files\Common Files\VST3`), a następnie w FL Studio wykonaj ponowne skanowanie wtyczek.

## Zależność

Projekt korzysta z [JUCE](https://github.com/juce-framework/JUCE) i [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) przez CMake FetchContent. Sprawdź licencję JUCE przed dystrybucją lub sprzedażą wtyczki.
