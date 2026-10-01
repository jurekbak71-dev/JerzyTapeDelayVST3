# ToneSnap — własny autotune VST3

Pierwszy prototyp efektu do korekcji monofonicznego wokalu. Wybierasz tonację i tryb skali, a wtyczka wykrywa wysokość dźwięku i przesuwa ją w stronę najbliższej dozwolonej nuty. Interfejs na tym etapie to ogólny panel parametrów JUCE.

## Parametry

- **Key** — tonacja od C do B.
- **Scale** — chromatyczna, durowa lub molowa.
- **Retune** — szybkość korekcji.
- **Amount** — siła korekcji.
- **Mix** — proporcja sygnału przetworzonego.

To własny, eksperymentalny algorytm czasu rzeczywistego: detektor YIN/autokorelacyjny oraz proste przesuwanie wysokości metodą zmiennej linii opóźniającej. To wersja do rozwijania i odsłuchu, a nie jeszcze jakość komercyjnych autotune'ów. W szczególności przy dużych korektach mogą być słyszalne artefakty; korekcja działa najlepiej na pojedynczym wokalu.

## Budowanie VST3 w Windows

Potrzebne są Visual Studio 2022 z C++ Desktop Development, CMake 3.22+ oraz dostęp do pobrania JUCE 8.0.6 podczas konfiguracji.

W PowerShellu, w katalogu `ToneSnap`:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Wtyczka pojawi się w `build/ToneSnap_artefacts/Release/VST3/ToneSnap.vst3`. Skopiuj ją do standardowego folderu VST3 (`C:\Program Files\Common Files\VST3`), a następnie w FL Studio wykonaj ponowne skanowanie wtyczek.

## Zależność

Projekt korzysta z [JUCE](https://github.com/juce-framework/JUCE) przez CMake FetchContent. Sprawdź licencję JUCE przed dystrybucją lub sprzedażą wtyczki.
