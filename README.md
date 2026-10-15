# Scratch Notes

Ein Notizblock für Noten: Desktop-Programm (JUCE/C++) zum Aufnehmen von Improvisationen auf E-Gitarre und E-Bass, mit einer oder mehreren Stimmen gleichzeitig oder per Overdub, je Stimme DI und bearbeitetes Signal. Danach schreibt das Programm die Aufnahme als Noten und Tabulatur auf und exportiert MIDI, MusicXML und Guitar Pro. Zielplattformen: Linux (ALSA/JACK) und Windows (ASIO/WASAPI), Roundtrip-Latenz ≤ 10 ms, Ziel 5 ms.

## Stand

AP0 (Projektgerüst): Das Programm öffnet das Audio-Gerät mit allen Ein- und Ausgängen, zeigt sie an und nennt in der Statusleiste Ein-, Ausgangs- und Roundtrip-Latenz. Ein Testton prüft die Ausgabe. ONNX Runtime (Tonerkennung, ab AP4) und Verovio (Notenanzeige, ab AP7) sind eingebunden und getestet.

## Planung

- [Lastenheft](docs/Lastenheft_Scratch_Notes.md)
- [Pflichtenheft](docs/Pflichtenheft_Scratch_Notes.md) – Arbeitspakete in Kapitel 8, Meilensteine in Kapitel 9
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

Verovio steht unter LGPLv3, ONNX Runtime unter MIT. Alle Komponenten von Drittanbietern mit ihren Lizenzen zeigt das Programm unter **Hilfe → Info und Lizenzen...**.
