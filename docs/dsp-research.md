# Tape Drive 1.1 — podstawy i pomiary DSP

## Źródła pierwotne

1. Jatin Chowdhury, *Real-Time Physical Modelling for Analog Tape Machines*, DAFx 2019:
   https://dafx.de/paper-archive/2019/DAFx2019_paper_3.pdf
   Sekcje 2.3.2 i 3.3.1: straty od odległości głowicy, szczeliny, grubości taśmy
   i prędkości zależą od częstotliwości. Sekcja 4.4: błędy prędkości wywołują
   modulację wysokości, realizowaną linią opóźniającą.
2. Universal Audio, *1176 vs. LA-2A: Understanding Two Legendary Compressors*:
   https://www.uaudio.com/blogs/ua/1176-vs-la-2a-understanding-two-legendary-compressors
   Optyczna komórka ma odpowiedź zależną od sygnału i czasu poprzedniej redukcji.
3. Universal Audio, *Tips & Tricks — Teletronix LA-2A Classic Leveler Plug-In Collection*:
   https://www.uaudio.com/blogs/ua/la-2a-collection-tips-tricks
   Atak około 10 ms; początkowy powrót około 60 ms dla połowy zmiany,
   pozostały powrót znacznie dłuższy i zależny od materiału.
4. Universal Audio, *Behind the Scenes at the UA Custom Shop*:
   https://www.uaudio.com/blogs/ua/behind-the-scenes-at-the-ua-custom-shop
   Charakter urządzenia obejmuje transformator wejściowy, nie tylko redukcję.

## Wnioski dla tej implementacji

Straty głowicy nie powinny być wyłącznie stałym filtrem. Poślizg, chwilowa
utrata kontaktu, zmiana pasma i spadek poziomu mają tworzyć skorelowane, losowe
zdarzenia. Implementacja jest behawioralnym przybliżeniem tych zależności;
nie rozwiązuje modelu Jilesa–Athertona z publikacji i nie jest pomiarem jednego
konkretnego egzemplarza magnetofonu. Częstość zdarzeń oraz ich zakres przy
wysokim Age celowo rozszerzono dla wyraźnego efektu zużycia.

Optyczna komórka i obwód audio to różne zjawiska. Reduction i Recovery
sterują dynamiką; Colour steruje obwodem nieliniowym i barwą. Detektor słucha
sygnału po redukcji, przed Makeup. Nie kopiuje dokładnie obwodu LA-2A;
zakres wysokiej redukcji i kontrola mieszania są rozszerzeniami użytkowymi.

## Pomiary deterministyczne

Test `TapeDriveCharacterTests`, 48 kHz, ustalone ziarno:

- sinus 997 Hz, 12 s, wyłącznie Wow 100%: 95. percentyl bezwzględnej
  zmiany wysokości około 49.5 centa;
- wyłącznie Flutter 100%: około 184.2 centa;
- Age 100%, Wow/Flutter 0%: 464 z 1100 okien 10 ms poniżej 60% poziomu
  odniesienia; najgłębsze okno około -23.0 dB;
- sinus 1 kHz, amplituda 0.5, Colour 0%, Reduction 95%:
  ustalona redukcja około 35.0 dB;
- Colour 85%, Reduction 0%: suma H2/H3 względem składowej podstawowej
  około 17.1% — pomiar barwy izolowanego kompresora, nie całego Tape Drive;
- 300 ms po krótkim/długim obciążeniu: około 11.6 / 14.2 dB pozostałej redukcji.

Sprawdzane są też: skuteczność Recovery, oryginalny sygnał przy Mix=0 i bypass,
nieprzesuwanie centrum stereo, niezależność od wielkości bufora, float/double,
próbkowanie 8–192 kHz, pełne ustawienia ekstremalne, stan i zgodność starych zapisów.
To pomiary syntetyczne. Odsłuch w FL Studio pozostaje osobnym testem użytkowym.
