# JERZY AUTO TUNE — VST3

Efekt do korekcji monofonicznego wokalu. Wybierasz tonację i tryb skali, a wtyczka wykrywa wysokość dźwięku i przesuwa ją w stronę najbliższej dozwolonej nuty. Interfejs ma własny układ: wybór tonacji i skali oraz pokrętła korekcji.

## Parametry

- **Key** — tonacja od C do B.
- **Scale** — chromatyczna (najbliższy półton), durowa lub molowa.
- **Retune** — szybkość korekcji.
- **Amount** — siła korekcji.
- **Mix** — proporcja sygnału przetworzonego.

Korekcja wysokości korzysta z Signalsmith Stretch (licencja MIT). Wykrywanie tonu używa pełnej, jednopróbkowej siatki opóźnień i interpolacji ułamkowej okresu, żeby ograniczyć skoki decyzji między nutami. Efekt zgłasza stałą latencję do hosta.

## Budowanie VST3 w Windows

Potrzebne są Visual Studio 2022 z C++ Desktop Development, CMake 3.24+ oraz dostęp do pobrania JUCE 8.0.6 i Signalsmith Stretch podczas konfiguracji.

W PowerShellu, w katalogu `ToneSnap`:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Wtyczka pojawi się w `build/JerzyAutoTune_artefacts/Release/VST3/JERZY AUTO TUNE.vst3`. Skopiuj ją do standardowego folderu VST3 (`C:\Program Files\Common Files\VST3`), a następnie w FL Studio wykonaj ponowne skanowanie wtyczek.

## Zależność

Projekt korzysta z [JUCE](https://github.com/juce-framework/JUCE) i [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) przez CMake FetchContent. Sprawdź licencję JUCE przed dystrybucją lub sprzedażą wtyczki.
