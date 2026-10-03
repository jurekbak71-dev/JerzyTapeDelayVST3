# Jerzy Tape Drive — Vector 1.1

Wtyczka VST3 dla Windows x64. Interfejs zbudowany od nowa, bez bitmap i plików
UIDescription. Zachowane są identyfikatory wtyczki i dotychczasowych parametrów. Nowe parametry
dopisano na końcu stanu; stare projekty można wczytać. Brzmienie DSP zostało
celowo rozszerzone, więc te same ustawienia mogą brzmieć mocniej.

## Interfejs

- Rozmiar początkowy: **880 × 560 px**. DPI nie mnoży ponownie wymiarów okna.
- Zapisany wcześniej za duży rozmiar jest ograniczany do obszaru roboczego ekranu.
- Przyciski **70 / 85 / 100 / 120%** są zawsze u góry. Dostępny jest też uchwyt
  do przeciągania prawego dolnego narożnika i zmiana rozmiaru przez host.
- Gałki, teksty, przyciski i wskaźniki są rysowane wektorowo. Po resize kontrolki
  otrzymują nowe prostokąty rysowania i kliknięć; CFrame nie jest powiększany
  transformacją. Nie ma starego edytora, fotorealistycznego tła ani szpul.
- Gałka: przeciąganie pionowe, kółko myszy, Shift dla precyzyjnej regulacji,
  podwójne kliknięcie albo Ctrl+klik dla resetu. Zmiany zgłaszają gesty automatyzacji.
- Przyciski wyboru cyklicznie zmieniają stan po kliknięciu.

Sekcje: kompresor optyczny na wejściu (Reduction, Colour, Recovery, Mix, Makeup, bypass, GR), taśma
(Drive, Gain, Contour), przedwzmacniacz (Off/Tube/Transistor, Drive), wyjście
(Level, Dry, bypass), Wow, Flutter, Age i filtry HPF/LPF z rezonansami.
Wskaźniki: Input, Output, Saturation.

## DSP

Tor mokry: wejście → kompresor optyczny → przedwzmacniacz → saturacja taśmy →
Wow/Flutter → zużycie taśmy → Level → filtry. Dry dodaje oryginalny sygnał.
Wow i Flutter korzystają z osobnych losowych trajektorii o różnych skalach czasu.
Dolna część zakresu jest subtelna, górna służy do wyraźnego niszczenia dźwięku.
Age dodaje poślizgi, krótkie zagniecenia, ubytki kontaktu z głowicą, zmienne
straty wysokich częstotliwości, oddychający szum i serie trzasków. Age wpływa
na transport nawet przy Wow=Flutter=0. Ruch taśmy jest wspólny dla stereo;
uszkodzenia ścieżek i szum mają niezależne składowe. Generator jest inicjowany
poza callbackiem audio; kolejne odtworzenia nie powtarzają tego samego przebiegu.
Prędkość głowicy odczytu ma ograniczenie zapobiegające odwróceniu kierunku.

Kompresor: detekcja ze sprzężeniem zwrotnym, próg zależny od Reduction,
miękkie kolano, szybka i wolna odpowiedź fotokomórki oraz pamięć długości
obciążenia. Maksymalna redukcja wynosi 36 dB (wskaźnik pokazuje rzeczywistą redukcję do 36 dB).
To model zachowania inspirowany kompresorami optycznymi, nie kopia układu LA-2A.

- **Reduction**: głębokość kompresji; górny zakres może bardzo obniżyć poziom.
- **Colour**: nasycenie transformatora i asymetryczna saturacja lampowa,
  harmoniczne parzyste/nieparzyste oraz wzmocnienie niskiego pasma. Działa
  również przy Reduction=0; 0% daje tor bez tego zabarwienia.
- **Recovery**: powrót i czas pamięci fotokomórki. Nadal zależy od materiału.
- **Mix**: równoległa domieszka całego toru kompresora, 0% = sygnał wejściowy.
- **Makeup**: ręczne wyrównanie poziomu po kompresji, od -12 do +12 dB.
- **Bypass**: pomija redukcję i kolorowanie, z łagodnym przejściem.

Nowe obwody nieliniowe używają antialiasingu przez funkcję pierwotną (ADAA).
Nie oznacza to, że cały dotychczasowy tor saturacji taśmy jest oversamplowany.
Stare stany bez Colour wczytują Colour=0; brak kompresora w bardzo starym stanie
włącza jego bypass. Numer i kolejność starych parametrów pozostają zachowane.

Przykład wyraźnie zużytej taśmy: Wow 65%, Flutter 50%, Age 80%, Dry 0%.
Przykład gęstego kompresora: Reduction 55%, Colour 60%, Recovery 65%, Mix 75%;
wyrównaj głośność Makeup i Level. To punkty wyjściowe, zależne od poziomu nagrania.

Dokumentacja podstaw projektu i wyników pomiarów: [docs/dsp-research.md](docs/dsp-research.md).

## Instalacja

Pobierz artefakt **JerzyTapeDrive-Vector-1.1-Windows-x64** z zakończonej kompilacji
GitHub Actions. Zamknij DAW i zastąp cały folder `JerzyTapeDrive.vst3` w
`C:\Program Files\Common Files\VST3`. W nowym panelu widnieje **VECTOR 1.1**.

## Kompilacja i weryfikacja

```sh
cmake -S . -B build -DVST3_SDK_ROOT=/path/to/vst3sdk -DSMTG_CREATE_PLUGIN_LINK=0
cmake --build build --config Release --target JerzyTapeDrive TapeDriveDSPTests TapeDriveCharacterTests
ctest --test-dir build -C Release --output-on-failure -R "TapeDrive(DSP|Character)"
```

Workflow Windows sprawdza kompilację, walidator VST3, regresje DSP i rzeczywiste
GUI w HWND: początkowe wymiary, DPI, ograniczenie zapisanego dużego rozmiaru,
przyciski powiększenia, wszystkie obszary parametrów, przeciąganie gałki,
gesty automatyzacji, bypass, uchwyt resize i zmianę okna bez callbacku VST3.
Test widocznego okna porównuje piksele pulpitu z oczekiwanym panelem i zapisuje
zrzuty. Jest to test w hoście Windows; nie uruchamia FL Studio.
