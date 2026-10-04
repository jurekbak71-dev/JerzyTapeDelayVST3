# Jerzy Tape Drive VST3

Projekt efektu taśmowego **Jerzy Tape Drive**.

## Tape Drive — Analog Vibes

![Podgląd układu interfejsu](docs/tapedrive-preview.png)

Panel Tape Drive nawiązuje do dostarczonego wzoru: drewniana obudowa, zużyty metal,
szpule, metalowe gałki i bursztynowe wskaźniki. Ilustracja pokazuje układ oraz
przykładowe ustawienia; nie jest zrzutem okna DAW.

- **DRIVE**: nasycenie taśmy. **OUTPUT**: poziom całego toru mokrego, razem z szumem.
- **DRY**: dodanie oryginalnego sygnału (dotychczasowy tryb addytywny).
- **WOW**: wolne, losowo zmieniające się odchylenia transportu.
- **FLUTTER**: niezależne, szybsze i nieregularne drżenie transportu.
- **TAPE AGE**: szum taśmy, losowe trzaski, krótkie ubytki sygnału i utrata góry.
  Przy 0% nie dodaje szumu ani trzasków. Artefakty pojawiają się także na ciszy.
- **OPTICAL INPUT COMPRESSOR**: miękkie kolano, detekcja połączona dla stereo,
  szybka i wolna faza powrotu z pamięcią poziomu. COMPRESSION reguluje ilość
  kompresji, MAKEUP poziom przed przedwzmacniaczem, GR pokazuje redukcję w dB.
- **PREAMP**: OFF / TUBE / TRANSISTOR; **GAIN** i **SHIFT** zachowują swoje funkcje.
- **HPF / LPF**: częstotliwości filtrów i rezonans Q na wyjściu.
- Lampka przy **BYPASS** świeci, gdy dana sekcja jest pomijana.
  Przełączniki zmieniają stan po kliknięciu; gałki obsługują przeciąganie,
  kółko myszy i reset do wartości domyślnej zgodnie z obsługą VSTGUI/DAW.
- **UI SIZE**: 75 / 100 / 125 / 150%, z uwzględnieniem skali ekranu.
  Ręczna zmiana rozmiaru dopasowuje cały panel i obszary kliknięć do rzeczywistego
  obszaru GUI w DAW, także gdy host pomija ograniczenia proporcji.

Tor mokry: wejście → kompresor optyczny → przedwzmacniacz → saturacja taśmy →
transport Wow/Flutter → zużycie taśmy → OUTPUT → filtry wyjściowe.
Transport jest wspólny dla kanałów stereo; szum, trzaski i ubytki są niezależne.
Wow/Flutter używają interpolowanych losowych trajektorii o zmiennym czasie,
zamiast cyklicznych sinusoid. Przy aktywnej modulacji tor mokry ma ruchome
opóźnienie około 4 ms; DRY pozostaje bez opóźnienia. To może dać interferencję
przy mieszaniu z sygnałem suchym, tak jak przy modulowanej taśmie.

## MX Analog Delay — dual multidelay

Projekt zawiera także osobny plugin **Jerzy MX Analog Delay**. Nie kopiuje znaków
towarowych ani obudów producentów; algorytmy i panele są autorską implementacją
inspirowaną klasycznymi rodzinami opóźnień sprzętowych.

Dwa niezależne silniki A/B mogą pracować w trybie **SERIES**, **PARALLEL** albo
**SPLIT L/R**. Każdy slot może wybrać jeden z siedmiu algorytmów i ma osobną,
automatycznie przełączaną stronę GUI:

- **MultiHead Reel** — cztery głowice playback/feedback, spacing, mechanika,
  zużycie, low-cut, spread i nasycenie.
- **Tape Echo** — tape age, wow, flutter, crinkle, bias, low contour, spring
  oraz trzy tryby maszyny.
- **Oil Can** — model dysku elektrostatycznego z viscosity, static, head mix,
  drift i ograniczonym zakresem czasu.
- **Tube Echo** — pojedyncza głowica, model przedwzmacniacza, record level,
  bias, mechanika, tape age i stereo spread.
- **BBD** — warianty 3205 / 3005 / MULTI, filtracja, modulacja, companding,
  szum układu i sprzężenie krzyżowe.
- **Doubletrack** — zakres od flangingu przez chorus do slapbacku,
  saturation, wobble, blend, type, width i auto-flange.
- **Dual Digital** — dwa tory opóźnienia z modelami 24/96, ADM i 12-bit,
  ratio, modulacją, cross-feedback, repeat dynamics i tone.

### Integracja z FL Studio

- tempo jest pobierane bezpośrednio z VST3 Process Context;
- Sync ma podziały od 1/1 do 1/16 i triol;
- wszystkie edytowalne parametry są publikowane jako stabilne parametry VST3
  z flagą automatyzacji, więc FL Studio może nagrywać ruchy gałek i tworzyć
  Automation Clips;
- wejście event/MIDI jest aktywne, a IMidiMapping mapuje typowe CC do Mix,
  Bypass, Time, Feedback, Pan i poziomów obu silników;
- Spillover pozwala zachować ogony po bypassie.

### Kompilacja MX Analog Delay

Workflow **Build Windows VST3** buduje oba pluginy i publikuje osobne artefakty.
Dla MX pobierz **JerzyMXAnalogDelay-Windows-x64** i skopiuj
`JerzyMXAnalogDelay.vst3` do `C:\Program Files\Common Files\VST3`.

Test DSP dla MX można uruchomić osobno:

```sh
c++ -std=c++17 -O2 -Isource tests/mxdelay_test.cpp -o mxdelay_test
./mxdelay_test
```

## Zgodność projektów

Dotychczasowe identyfikatory parametrów i identyfikatory wtyczek Tape Drive
pozostają stałe. MX Analog Delay ma osobne UID i osobny stan, więc nie zastępuje
istniejącego Tape Drive.

## Kompilacja i pobieranie

Workflow **Build Windows VST3** buduje Tape Drive i MX Analog Delay dla Windows x64,
uruchamia testy DSP oraz natywny test GUI Tape Drive, a następnie publikuje paczki
VST3. W zakładce **Actions** wybierz odpowiedni artefakt i rozpakuj plugin do
`C:\Program Files\Common Files\VST3`. Następnie przeskanuj wtyczki w DAW.

Lokalnie potrzebne są CMake 3.25+, C++17 i oficjalny Steinberg VST3 SDK
z submodułami. SDK można wskazać przez `VST3_SDK_ROOT`.

```sh
cmake -S . -B build -DVST3_SDK_ROOT=/path/to/vst3sdk -DSMTG_CREATE_PLUGIN_LINK=0
cmake --build build --config Release --target JerzyTapeDrive JerzyMXAnalogDelay TapeDriveDSPTests MXDelayDSPTests
ctest --test-dir build -C Release --output-on-failure
```


## MX Analog Delay 0.4.1 — GUI / FL Studio integration

- **MultiHead Reel** ma teraz niezależną panoramę dla każdej z czterech głowic. Parametry Head 1–4 Pan są automatyzowalne i zapisywane w stanie projektu.
- GUI używa skalowalnego edytora 1280×720 z obsługą DPI i resize hosta; przyciski 75 / 100 / 125 / 150% pozwalają wymusić wygodny rozmiar.
- Layout A/B został przebudowany tak, aby wspólne parametry slotu nie nachodziły na strony algorytmów.
- Tło jest rysowane proceduralnie jako oksydowana błękitno-stalowa blacha, więc zachowuje ostrość przy skalowaniu.
- Przełączniki ON/SYNC/PLAY/FB dostały wskaźniki LED.
- Prawy przycisk myszy na każdej gałce wraca do wartości domyślnej parametru.
- Automatyzacja VST3 jest obsługiwana na offsetach próbek wewnątrz bloku, co poprawia szybkie przebiegi Automation Clips w FL Studio.
- MIDI: poza dotychczasowymi mapowaniami CC dodano CC20–27 dla panoram czterech głowic A/B oraz CC28–43 dla włączania głowic playback/feedback.
- Stan v2 zachowuje zgodność z projektami zapisanymi przez wersję 0.4.0; stare sesje dostają domyślne pozycje panoramy głowic.
