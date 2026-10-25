# Scratch Notes

Ein Notizblock für Noten: Desktop-Programm (JUCE/C++) zum Aufnehmen von Improvisationen auf E-Gitarre und E-Bass, mit einer oder mehreren Stimmen gleichzeitig oder per Overdub, je Stimme DI und bearbeitetes Signal. Danach schreibt das Programm die Aufnahme als Noten und Tabulatur auf und exportiert MIDI, MusicXML und Guitar Pro. Zielplattformen: Linux (ALSA/JACK) und Windows (ASIO/WASAPI), Roundtrip-Latenz ≤ 10 ms, Ziel 5 ms.

## Stand

AP0 (Projektgerüst): Das Programm öffnet das Audio-Gerät mit allen Ein- und Ausgängen, zeigt sie an und nennt in der Statusleiste Ein-, Ausgangs- und Roundtrip-Latenz. Ein Testton prüft die Ausgabe. ONNX Runtime (Tonerkennung, ab AP4) und Verovio (Notenanzeige, ab AP7) sind eingebunden und getestet.

## Planung

- [Lastenheft](docs/Lastenheft_Scratch_Notes.md)
- [Pflichtenheft](docs/Pflichtenheft_Scratch_Notes.md) – Arbeitspakete in Kapitel 8, Meilensteine in Kapitel 9
- [GUI-Entwurf v0.1](docs/GUI-Entwurf_v0.1.png) ([hell](docs/GUI-Entwurf_v0.1_hell.png))
- [Latenzmessung](docs/Latenzmessung.md) – Vorgehen für Q-01 und Q-02
- Fortschritt: Issues und Meilensteine in diesem Repository

## Bauen

Voraussetzungen: CMake ≥ 3.22, Ninja, ein C++20-Compiler (GCC/Clang unter Linux, Visual Studio (MSVC) unter Windows). Unter Linux zusätzlich die JUCE-Abhängigkeiten:

```sh
sudo apt install libasound2-dev libjack-jackd2-dev libfreetype-dev libfontconfig1-dev \
  libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev \
  libxrandr-dev libxrender-dev libgl1-mesa-dev
```

JUCE, GoogleTest und Verovio liegen als Git-Submodule unter `external/`:

```sh
git clone --recurse-submodules https://github.com/ladych/ScratchNotes.git
# bei bestehendem Klon: git submodule update --init --recursive
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

**ONNX Runtime:** CMake lädt beim Konfigurieren das offizielle Paket der festgelegten Version (1.31.0) von GitHub und legt die Bibliothek neben das Programm und die Tests. Ohne Internet: Paket selbst entpacken und `-DSNOTES_ONNXRUNTIME_DIR=/pfad/zu/onnxruntime` angeben (das Verzeichnis mit `include/` und `lib/`).

**Windows mit ASIO:** Steinberg stellt das ASIO-SDK seit Oktober 2025 unter GPLv3 bereit, das ist mit unserer AGPLv3 vereinbar. CMake lädt die festgelegte Version (2.3.4, mit SHA-256 geprüft) beim Konfigurieren von Steinberg; sie liegt nicht im Repository. Ohne Internet: SDK selbst entpacken und `-DSNOTES_ASIO_SDK_DIR=C:/pfad/zum/asiosdk` angeben (das Verzeichnis mit `common/iasiodrv.h`); `-DSNOTES_ASIO=OFF` baut ohne ASIO. Die CI stellt das fertige Windows-Programm als Artefakt `ScratchNotes-Windows` bereit.

Beim ersten Start wählt das Programm ASIO, falls ein ASIO-Treiber mit Eingängen installiert ist, sonst WASAPI. Unter Linux nimmt es JACK, wenn ein JACK-Server läuft (auch das JACK von PipeWire), sonst ALSA; das ALSA-Gerät „default“ öffnet sich oft erst ab 512 Samples Buffer. Fällt das gewählte Gerät aus (z. B. Interface abgezogen), läuft es über den nächsten Treiber weiter und kehrt zum gewählten Gerät zurück, sobald es wieder da ist; die Statusleiste zeigt das an. **Audio → Audio-Einstellungen...** (Strg+,) wählt Treiber, Gerät, Samplerate, Buffer und die aktiven Ein- und Ausgänge.

**Tonerkennung:** Nach jeder Aufnahme erkennt das Programm im Hintergrund die Noten der neuen Takes aus der DI-Spur (Basic Pitch, eingebaut); **Noten → Noten erkennen** (Strg+Umschalt+T) erkennt die aktiven Takes neu, die Statusleiste zeigt den Fortschritt. Die Noten liegen als `<DI-Datei>.notes.csv` neben der Aufnahme. Ohne Programm geht das mit dem Kommandozeilen-Werkzeug, das neben dem Programm gebaut wird:

```bash
scratchnotes-transcribe --profile bass5-gs --out ergebnisse "Bass Take 1 DI.wav"
# schreibt ergebnisse/Bass Take 1 DI.notes.csv und .mid; --help nennt die Instrumentprofile
scratchnotes-transcribe --out ergebnisse "Gitarre Take 1 DI.wav" "Bass Take 1 DI.wav"
# gemeinsames Beat-Raster: Gitarre Take 1 DI.beats.csv und .meter.csv, je Stimme .events.csv und .tab.txt,
# die Partitur aller Stimmen als Gitarre Take 1 DI.musicxml
# --separate rechnet für jede Datei ein eigenes Raster
```

**Rhythmus:** Sobald keine Tonerkennung mehr läuft, rechnet das Programm aus den aktiven Takes aller Stimmen ein gemeinsames Beat-Raster (Tempo, Taktart mit Konfidenz, Taktanfang), räumt die Noten auf (Anschläge zusammenfassen, Nachklingen und Vorschläge erkennen, Riff oder Melodie) und quantisiert sie auf Achtel, Sechzehntel oder Triolen. Die Spuren zeigen die Schläge als Linien, Taktanfänge kräftiger; die Statusleiste nennt Tempo und Taktart. Die Ereignisse liegen als `<DI-Datei>.events.csv` neben der Aufnahme.

**Tabulatur:** Jede Note bekommt Saite und Bund nach dem Instrument-Profil der Stimme. Eine dynamische Programmierung sucht den günstigsten Griffweg: Lagenwechsel kosten je Bund (nach einer Pause weniger), Akkorde dürfen höchstens vier Bünde spreizen, leere Saiten lassen die Hand, wo sie ist. Die Tab steht in den Spalten `tab` und `grace_tab` der `.events.csv` (Saite:Bund, Saite 1 = höchste); das Kommandozeilen-Werkzeug schreibt zusätzlich eine Text-Tab `<Name>.tab.txt` zum Nachspielen.

**Noten:** Aus den Ereignissen aller Stimmen entsteht eine Partitur in der gewählten Taktart: je Stimme ein System mit Noten (Violinschlüssel bzw. Bassschlüssel eine Oktave tiefer) und eines mit Tabulatur in der Stimmung des Profils. Stille vor der ersten Note wird abgeschnitten; Takt 1 beginnt mit der ersten Eins, Noten davor bilden einen Auftakt. Noten über Schläge und Taktstriche werden gebunden, Achtel und kürzer je Schlag gebalkt, Triolen geklammert. Die Partitur liegt als `score.musicxml` im Projektordner, Verovio zeichnet sie in der Notenansicht. Oben rechts wählen **Spuren**, **Noten** und **Beides** die Ansicht; die Wahl wird mit dem Projekt gespeichert.

**Korrektur von Taktart und Beat-Raster:** Die Auswahl neben der Song-Position zeigt die Taktart-Deutungen der Analyse mit ihrer Wahrscheinlichkeit; eine Wahl übernimmt Taktart und ersten Downbeat. Über den Spuren liegt ein Beat-Lineal: einen Beat ziehen verschiebt ihn, ein Doppelklick fügt einen ein, ein Rechtsklick löscht einen oder lässt dort einen Takt beginnen. Nach jeder Änderung werden die Ereignisse aller Stimmen auf dem neuen Raster neu quantisiert, ohne neue Tonerkennung, und Partitur und `score.mid` neu geschrieben. Das bearbeitete Raster bleibt auch bei späteren Notenerkennungen erhalten; **Noten → Beat-Raster neu erkennen** verwirft die Korrekturen.

**Mitklopfen:** Während der Wiedergabe jeden Schlag mit der Taste B mitklopfen (Noten → Beat mitklopfen). Beim Stopp ersetzen die Schläge die Beats in ihrem Bereich, bis eine halbe Schlaglänge davor und danach; die Ausgangslatenz des Geräts wird abgezogen. Danach werden die Noten neu gesetzt.

**Export:** Nach jeder Notenerkennung liegt neben `score.musicxml` auch `score.mid`. **Datei → Als MIDI exportieren…** und **Als MusicXML exportieren…** kopieren sie an einen frei gewählten Ort. Die MIDI-Datei (Typ 1, 960 Ticks je Viertel) hat eine Tempospur mit Taktart und einem Tempowechsel auf jedem Schlag, danach je Stimme eine Spur mit Namen, Gitarren- bzw. Bass-Klang und eigenem Kanal (Kanal 10 bleibt frei). Sie läuft synchron zur Aufnahme: MIDI-Zeit 0 ist der Projektanfang, der Vorlauf bis zur Partitur füllt ganze Takte, sodass Takt 1 der Partitur in der DAW auf einen Taktstrich fällt. Vorschläge erklingen als Zweiunddreißigstel kurz vor ihrem Akkord. Die MusicXML-Datei trägt Tabulatur und das mittlere Tempo als Metronomangabe.

`tools/export-basic-pitch-reference.py` und `tools/export-rhythm-reference.py` erzeugen die Python-Referenzwerte in `tests/data/transcription/` und `tests/data/rhythm/`, mit denen die Tests die C++-Portierung prüfen (E-12). `tools/evaluate-transcription.py <CLI> <Aufnahmen> <Ausgabe>` misst Q-06/Q-07 an den PoC-Aufnahmen und schreibt `metrics.json`; `tools/check-low-strings.py <CLI> <Aufnahmen> <Ausgabe>` prüft Q-08 an den Aufnahmen in `g#guitar/` und `g#bass/` (tiefste leere Saite, mit pYIN als Vergleich).

CLion liest `CMakePresets.json` direkt; die Presets `coverage`, `asan` und `tsan` bauen nur die Tests.

## Qualitätsregeln

Übernommen aus [Drum Programmer](https://github.com/ladych/DrumProgrammer) (E-01 bis E-09):

- Tests mit GoogleTest/GoogleMock, Testnamen mit Anforderungs-ID (E-01, E-09)
- 100 % Zeilen- und Branch-Abdeckung für `src/`, Ausnahmen mit Begründung in `coverage-exclusions.txt` (E-02, E-03); lokal: `cmake --preset coverage && cmake --build --preset coverage && ctest --preset coverage && tools/check-coverage.py build/coverage` (gcovr ≥ 8)
- `clang-format` und `tools/run-clang-tidy.sh <build-dir>` ohne Befund, Warnungen sind Fehler (E-08)
- Composition Root in `src/app/AppComposition.cpp` (E-05)
- Im Audio-Callback keine Allokation, keine Locks (Q-04), geprüft von `RealtimeAllocationTest`

Die CI (GitHub Actions) prüft all das bei jedem Push: Linux- und Windows-Build mit Tests, Coverage-Gate, Address/UB- und Thread-Sanitizer, clang-format und clang-tidy.

## Sprachen

Die Oberfläche gibt es auf Deutsch und Englisch; **Hilfe → Sprache** wählt sie, Default ist die Sprache des Betriebssystems, sonst Englisch. Die Wahl gilt ab dem nächsten Start und liegt als `language.txt` im Einstellungsordner. **Hilfe → Farbschema** wählt hell, dunkel oder die Systemeinstellung.

Im Code stehen alle Texte englisch in `i18n::tr("...")` (oder `i18n::marked("...")` in Tabellen). Jede weitere Sprache ist eine Datei `resources/Languages/<ISO-639-1-Code>.txt` im Format von `juce::LocalisedStrings`, die beim Bauen ins Programm kommt; der Kopf von [resources/Languages/de.txt](resources/Languages/de.txt) beschreibt, wie eine neue Sprache angelegt wird. Fehlt eine Übersetzung, erscheint der englische Text. `tools/check-translations.py` (auch als Test `TranslationsComplete`) prüft, dass jede Datei jeden Text enthält.

## Lizenz

GNU Affero General Public License v3.0, siehe [LICENSE](LICENSE). Die Lizenz folgt aus der Open-Source-Lizenz von JUCE.

Verovio steht unter LGPLv3, ONNX Runtime unter MIT, das Modell von Basic Pitch unter Apache 2.0 (`resources/Models/LICENSE`). Alle Komponenten von Drittanbietern mit ihren Lizenzen zeigt das Programm unter **Hilfe → Info und Lizenzen...**.
