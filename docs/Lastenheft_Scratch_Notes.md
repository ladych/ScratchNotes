# Lastenheft: Scratch Notes

**Status:** v0.3, alle Fragen aus Abschnitt 7 entschieden
**Stand:** 15.10.2026
**Projekttyp:** Nebenprojekt, Entwicklung mit KI-Unterstützung
**Grundlagen:** Proof of Concept „Transkription“ (`ZUSAMMENFASSUNG.md`, Python), Projekt Drum Programmer (`ladych/DrumProgrammer`, JUCE/C++)

---

## 1. Projektübersicht

### 1.1 Zielsetzung
Scratch Notes ist ein Notizblock für Musiker: Eine Improvisation auf einem oder mehreren Instrumenten wird aufgenommen, und das Programm schreibt sie danach als Noten und Tabulatur auf. Im Vordergrund steht moderne Musik mit elektrischen Instrumenten, vor allem E-Gitarre und E-Bass.

Weil frei improvisiert wird, ohne Metronom und ohne Zählen, trifft nicht jede Stimme genau den Takt. Das Programm muss deshalb Tempo und Taktart selbst schätzen und Tempo-Schwankungen zulassen, statt auf ein starres Raster zu quantisieren.

### 1.2 Auftraggeber / Entwickler
Einzelentwickler (Alex), Hobby-/Nebenprojekt. Code wird mit KI-Unterstützung geschrieben; die Qualitätsregeln aus Abschnitt 4 sichern das ab.

### 1.3 Zeitrahmen
- ca. 10,5 Std./Woche (entschieden 15.10.2026)
- Grobschätzung in Abschnitt 6, Detailplanung im Pflichtenheft

---

## 2. Technischer Stack

| Bereich | Wahl | Begründung |
|---|---|---|
| Framework | **JUCE 8** (C++) | Audio-I/O, GUI, Dateiformate in einem Framework; bewährt in Drum Programmer |
| Sprache | **C++20** | wie Drum Programmer |
| IDE / Build | CLion, CMake mit Presets, Ninja | aus Drum Programmer übernommen |
| CI | GitHub Actions, Linux und Windows | aus Drum Programmer übernommen |
| Tests | GoogleTest / GoogleMock | aus Drum Programmer übernommen |
| Tonerkennung | **Basic Pitch** (Spotify, Apache 2.0) als ONNX-Modell über **ONNX Runtime** (MIT) | im PoC bestes Ergebnis, mehrstimmig; ONNX Runtime hat eine C++-API für Windows und Linux |
| Beat-Tracking | eigene C++-Umsetzung: Onset-Stärke + dynamische Programmierung (Verfahren wie librosa) | im PoC fast gleich gut wie madmom (PoC 5.6); madmom gibt es nicht für C++, und seine Modelle stehen unter CC BY-NC-SA (nicht mit AGPLv3 vereinbar) |
| Notenbild | **Verovio** (C++, LGPLv3) | liest MusicXML/MEI, kann Tabulatur, rendert SVG; im PoC schon verwendet |
| Versionierung | Git, GitHub | — |

---

## 3. Funktionale Anforderungen

### 3.1 Aufnahme
- [ ] Ein Projekt hat eine oder mehrere **Stimmen** (z. B. Gitarre 1, Bass)
- [ ] Jede Stimme hat **1 oder 2 Kanäle**: das **DI-Signal** des Instruments und optional das **bearbeitete Signal** (Amp, Effekte), jeweils einem Eingang des Audio-Interfaces zugeordnet
- [ ] Mehrere Stimmen können **gleichzeitig** aufgenommen werden, so viele, wie das Interface Eingänge hat
- [ ] Aufnahme direkt auf die Platte als WAV (32 Bit float, Samplerate des Geräts), ohne Speicher-Allokation oder Dateizugriff im Audio-Thread
- [ ] Pegelanzeige je Eingang, Warnung bei Übersteuerung
- [ ] Jede Aufnahme ist ein **Take**; eine Stimme kann mehrere Takes haben, einer davon ist aktiv
- [ ] Undo/Redo, ein Take ist ein Undo-Schritt

### 3.2 Wiedergabe und Overdub
- [ ] Vorhandene Stimmen abspielen und gleichzeitig eine neue aufnehmen (**Overdub**)
- [ ] Lautstärke, Stumm und Solo je Stimme; abgespielt wird das bearbeitete Signal, falls vorhanden, sonst das DI
- [ ] **Latenzausgleich**: neue Takes werden um die gemessene Ein- plus Ausgangslatenz verschoben, damit sie synchron zu den gehörten Stimmen liegen
- [ ] Optionales Software-Monitoring des Eingangs je Stimme (Frage 7.2)
- [ ] Optionaler Klick; Standard ist aus, weil frei improvisiert wird (Frage 7.5)

### 3.3 Transkription
- [ ] Startet nach der Aufnahme im Hintergrund, die Oberfläche bleibt bedienbar; Fortschritt wird angezeigt
- [ ] Töne werden **immer aus dem DI-Kanal** erkannt (PoC: aus dem bearbeiteten Signal stimmen nur 41 % der Noten)
- [ ] **Tempo und Beats** werden aus allen Spuren gemeinsam bestimmt; alle Stimmen eines Projekts teilen sich ein **Beat-Raster** mit Tempokurve (Frage 7.4)
- [ ] Metrische Ebene erst global festlegen, dann lokale Tempo-Drift zulassen (PoC-Erkenntnis 3)
- [ ] **Taktart** als Vorschlag mit Konfidenz (3/4 bis 7/4), mehrere Deutungen; Plausibilitätsprüfung über wiederkehrende Phrasen
- [ ] **Aufräumen** nach Spielweise riff / melody, automatisch gewählt (Geisternoten, Obertöne, Nachklang, mitschwingende Leersaiten, Slide-Ansätze)
- [ ] **Quantisierung relativ zu den Beats**, pro Beat bestes Raster aus Viertel / Achtel / Triole / Sechzehntel
- [x] **Entscheidung (15.10.2026):** **Instrument-Profil** je Stimme mit Saitenzahl und Stimmung. MVP-Profile (Alex' Instrumente):

| Profil | Saiten (tief → hoch) | tiefster Ton |
|---|---|---|
| E-Gitarre 6, E-Standard | E2 A2 D3 G3 B3 E4 | E2 (82,4 Hz) |
| E-Gitarre 7, G#-Standard (C#-Standard + 7. Saite) | G#1 C#2 F#2 B2 E3 G#3 C#4 | G#1 (51,9 Hz) |
| E-Bass 4, E-Standard | E1 A1 D2 G2 | E1 (41,2 Hz) |
| E-Bass 5, G#-Standard | G#0 C#1 F#1 B1 E2 | G#0 (26,0 Hz) |

  - 5-Saiter: C#-Standard mit tiefer G#-Saite, passend zur 7-Saiter-Gitarre (bestätigt 15.10.2026)
  - Das Profil begrenzt den erlaubten Tonumfang (Filter gegen Obertöne und Fehlerkennungen) und bestimmt die Tabulatur
- [ ] **Tabulatur**: Saite und Bund je Note zuordnen, Griffweg optimieren
- [ ] Ergebnisse der Transkription sind reproduzierbar: gleiche Aufnahme und Einstellungen ergeben das gleiche Notenbild

### 3.4 Anzeige und Korrektur
- [ ] Noten und Tabulatur je Stimme, Partitur-Ansicht aller Stimmen untereinander
- [ ] Mitlaufende Position beim Abspielen (Audio und Noten synchron)
- [ ] **Taktart-Deutung per Klick wählen**
- [ ] **Beat-Raster korrigieren**: Beats verschieben, einfügen, löschen, oder durch Mitklopfen neu setzen; die Noten werden danach neu quantisiert
- [ ] Einzelne Noten korrigieren: löschen, Tonhöhe, Dauer, Saite/Bund (Umfang siehe Frage 7.7)
- [ ] Transkription nachspielen (Synthese) und A/B-Vergleich mit dem Original

### 3.5 Export
- [ ] **MIDI** (SMF Typ 1): eine Spur je Stimme, Tempo-Map aus dem Beat-Raster, Taktart; läuft in einer DAW synchron zur Aufnahme
- [ ] **MusicXML** mit Tabulatur-Angaben (Saite/Bund), öffnet in MuseScore und Guitar Pro
- [ ] **Guitar Pro**: zuerst über den MusicXML-Import von Guitar Pro; ein eigener .gp-Export ist Kann (Frage 7.8)
- [ ] Optional: Notenbild als PDF/SVG, Audio der Stimmen als WAV

### 3.6 Projektverwaltung
- [ ] Ein Projekt ist ein **Ordner** mit der Projektdatei (XML) und einem Unterordner für die Audio-Dateien, damit große Aufnahmen nicht in der Projektdatei liegen und Projekte portabel bleiben (Frage 7.9)
- [ ] Neu, Öffnen, Speichern, Speichern unter; Nachfrage bei ungespeicherten Änderungen
- [ ] Import vorhandener WAV-Dateien als Take (z. B. ältere Aufnahmen oder Reaper-Spuren)

### 3.7 Audio-Treiber
- [ ] Linux: ALSA, JACK (auch PipeWire-JACK)
- [ ] Windows: ASIO, WASAPI als Fallback
- [ ] Geräteausfall wird abgefangen (Verhalten wie Drum Programmer)

---

## 4. Nicht-funktionale Anforderungen

- **Latenz:** Round-Trip (Eingang bis Ausgang) **≤ 10 ms, Ziel 5 ms** mit geeigneter Hardware (ASIO/JACK, Audio-Interface). Zur Einordnung: Bei 48 kHz ergeben 128 Samples Puffer rechnerisch 2 × 2,7 ms plus Wandlerlatenz, also etwa 6–7 ms; für 5 ms braucht es 64 Samples bei 48 kHz oder 128 bei 96 kHz. Puffergröße und Samplerate sind einstellbar, die Statusleiste zeigt die Latenz.
- **Stabilität:** keine Aussetzer bei Aufnahme und Wiedergabe; ein Take geht auch bei einem Absturz nicht verloren (WAV wird fortlaufend geschrieben)
- **Transkriptionszeit:** höchstens so lang wie die Aufnahme auf einem aktuellen Desktop-PC (PoC in Python: 124 s für 132 s inkl. Export)
- **Genauigkeit:** mindestens so gut wie der PoC auf denselben drei Aufnahmen (Abschnitt 5.1 der Zusammenfassung); gemessen in automatischen Vergleichstests
- **Offline:** läuft ohne Internet und ohne Cloud-Dienst, ohne eigenes Training
- **Plattformen:** Linux (Entwicklung, Linux Mint) und Windows 10/11 64 Bit
- **Lizenz:** nur Open-Source-Bibliotheken; Produkt unter AGPLv3 (folgt aus JUCE)
- **Bedienbarkeit:** schnell vom Einstecken zur Aufnahme, wie ein Notizblock; keine DAW-Komplexität
- **Sprachen:** Deutsch und Englisch (i18n aus Drum Programmer)
- **Entwicklungsvorgaben:** wie Drum Programmer: TDD, 100 % Zeilen- und Branch-Abdeckung der Logik mit begründeten Ausnahmen (Humble Object), Clean Code mit clang-format/clang-tidy, Warnungen als Fehler, Constructor Injection mit einer Composition Root, im Audio-Callback statische DI. Zusätzlich für die Transkription: Regressions-Tests gegen Referenz-Aufnahmen.

---

## 5. Technische Rahmenbedingungen / bekannte Einschränkungen

- **ASIO-SDK:** von Steinberg unter GPLv3, wird wie bei Drum Programmer beim Konfigurieren geladen, nicht im Repository.
- **ONNX Runtime:** fertige Binärpakete je Plattform werden beim Bauen geladen (Größe ca. 10–20 MB); das Basic-Pitch-Modell (ca. 1 MB) wird ins Programm eingebaut.
- **madmom** wird nicht verwendet: kein C++, und die trainierten Modelle stehen unter CC BY-NC-SA 4.0.
- **Guitar Pro:** das Format ist nicht offen dokumentiert. Guitar Pro 7/8 (.gp) ist ein ZIP mit XML (GPIF), das andere Projekte (z. B. alphaTab) nachgebaut haben. Deshalb zuerst MusicXML.
- **Tiefe Töne:** Basic Pitch erkennt nur Töne ab A0 (27,5 Hz). Die tiefe G#0-Saite des 5-Saiters (26,0 Hz) liegt darunter, und auch E1 bis etwa C2 liegen am unteren Rand des Modells, wo es erfahrungsgemäß unsicherer ist. Geplante Abhilfe für Bass-Profile: die DI-Spur für die Analyse eine Oktave höher rechnen (doppelte Abspielgeschwindigkeit, Zeiten danach halbieren) und das Ergebnis eine Oktave zurückschieben. Ob das reicht, zeigen erst Testaufnahmen mit 7-Saiter und 5-Saiter-Bass; der PoC hat nur 6-Saiter in E-Standard geprüft.
- **Transkription ist nie fehlerfrei**, besonders bei Bends, Vibrato, Slides und gehaltenen Tönen unter einer Melodie (PoC Abschnitt 7). Die Korrektur durch den Menschen (3.4) ist deshalb Kernfunktion, nicht Zusatz.
- **Threading:** Audio-Thread, Platten-Schreiber, Transkription im Hintergrund und GUI-Thread strikt getrennt; Snapshot-Prinzip und lock-freie Queues aus Drum Programmer.

### 5.1 Übernahme aus Drum Programmer

| Modul | Verwendung |
|---|---|
| CMake-Presets, CI, Coverage-Gate, clang-format/clang-tidy, Sanitizer, ASIO-SDK-Download | komplett übernehmen |
| `audio/DriverChoice`, `audio/DeviceRecovery`, `app/AudioDeviceSupervisor` | um Eingänge erweitern |
| `engine/SpscQueue`, `engine/SnapshotExchange` | unverändert |
| `engine/Clock`, `engine/TempoMath`, `engine/Metronome` | Transport, Klick |
| `engine/BackingTrackPlayer`, `engine/Resampler` | Grundlage für die Wiedergabe der Stimmen |
| `io/MidiFileWriter`, `io/SettingsStore`, `io/StdFileSystem`, `io/ProjectRepository` | um Tempo-Map und Typ 1 erweitern |
| `i18n/` | unverändert |
| ValueTree-Modell mit Undo, Statusleiste mit Latenzanzeige, `docs/Latenzmessung.md` | Muster übernehmen |

Die Module werden in das neue Repository kopiert, nicht als gemeinsame Bibliothek geteilt. Eine gemeinsame Bibliothek lohnt sich erst, wenn beide Programme länger parallel gepflegt werden.

---

## 6. Grobe Meilensteine / Roadmap

| Phase | Inhalt | Geschätzter Aufwand |
|---|---|---|
| 1 | Setup aus Drum Programmer: Repo, CMake, CI, Tests, Audio-Gerät mit Ein- und Ausgängen | 1 Woche |
| 2 | Aufnahme-Engine: Stimmen mit 1–2 Kanälen, Platten-Schreiber, Pegel, Takes, Projektordner | 2–3 Wochen |
| 3 | Wiedergabe und Overdub, Latenzausgleich, Latenzmessung | 2–3 Wochen |
| 4 | Transkription in C++: Basic Pitch (ONNX), Beat-Tracking, Taktart, Aufräumen, Quantisierung, Vergleichstests gegen den PoC | 4–6 Wochen |
| 5 | Tabulatur: Saite/Bund, Griffweg, Instrument-Profile | 1,5–2 Wochen |
| 6 | Notenanzeige mit Verovio, Partitur, mitlaufende Position | 2–3 Wochen |
| 7 | Korrektur: Taktart wählen, Beat-Raster, Noten | 2–4 Wochen |
| 8 | Export MIDI, MusicXML (Guitar Pro über MusicXML) | 1–2 Wochen |
| 9 | Windows/ASIO-Tests, Stabilisierung, Release | 2–3 Wochen |

**MVP** (eine Stimme mit DI + bearbeitet aufnehmen, eine zweite per Overdub, transkribieren, Noten und Tab anzeigen, als MIDI und MusicXML exportieren; Linux): Phasen 1–6 und 8 in einfacher Form, laut Pflichtenheft ca. 21 Wochen
**Version 1.0** (mehrere Stimmen gleichzeitig, Korrektur-Funktionen, Windows mit ASIO, stabil): laut Pflichtenheft ca. 28 Wochen

*Grobschätzung, die verbindliche Planung mit Arbeitspaketen folgt im Pflichtenheft.*

---

## 7. Offene Fragen (zur weiteren Planung)

Zu jeder Frage steht ein Vorschlag. Ein „ok“ übernimmt ihn.

- [x] **7.1 Woher kommt das bearbeitete Signal?** Aus Hardware (Modeler, Amp, Pedale) über einen zweiten Eingang, oder soll Scratch Notes selbst Plugins (VST3, z. B. Amp-Simulationen) laden? → *Vorschlag: Hardware; Plugin-Hosting nicht in v1.0, weil es den Umfang stark vergrößert.* → **Entschieden 15.10.2026:** Hardware, kein Plugin-Hosting in v1.0
- [x] **7.2 Monitoring:** Hört der Spieler sich über das Interface (Direct Monitoring, ohne Latenz) oder über Scratch Notes (Software-Monitoring)? → *Vorschlag: beides; Software-Monitoring je Stimme ein- und ausschaltbar. Das 5-ms-Ziel gilt für Software-Monitoring und Overdub.* → **Entschieden 15.10.2026:** beides, Software-Monitoring je Stimme schaltbar
- [x] **7.3 Mehrere Stimmen:** gleichzeitig (mehrere Musiker live), nacheinander per Overdub, oder beides? → *Vorschlag: beides; MVP mit Overdub, gleichzeitige Aufnahme in v1.0.* → **Entschieden 15.10.2026:** beides, schon im MVP (Proberaum mit zwei Gitarristen)
- [x] **7.4 Gemeinsames Tempo:** Ein Beat-Raster für alle Stimmen eines Projekts, oder eins je Stimme? → *Vorschlag: ein gemeinsames Raster, damit alle Stimmen in einer Partitur und einer MIDI-Datei zusammenpassen; bei Overdub folgt der zweite Spieler ja der ersten Aufnahme.* → **Entschieden 15.10.2026:** ein gemeinsames Beat-Raster je Projekt
- [x] **7.5 Klick:** Soll es optional einen Klick geben (fest oder aus der Tempokurve der ersten Stimme)? → *Vorschlag: optional, Standard aus.* → **Entschieden 15.10.2026:** optionaler Klick, Standard aus
- [x] **7.6 Instrumente im MVP:** → *Vorschlag: E-Gitarre 6 Saiten (Standard, Drop D, Halbton tiefer) und E-Bass 4/5 Saiten; 7-Saiter, Klavier und weitere Instrumente später.* → **Entschieden 15.10.2026:** Profile siehe 3.3 (Gitarre 6 E-Standard, Gitarre 7 G#-Standard, Bass 4 E-Standard, Bass 5 G#-Standard)
- [x] **7.7 Umfang der Notenbearbeitung:** → *Vorschlag: Taktart wählen, Beat-Raster korrigieren, Note löschen, Tonhöhe/Dauer/Saite ändern. Kein vollständiger Noteneditor; dafür gibt es den Export nach MuseScore und Guitar Pro.* → **Entschieden 15.10.2026:** wie vorgeschlagen, kein vollständiger Noteneditor
- [x] **7.8 Guitar Pro:** Reicht für v1.0 der Weg über MusicXML? → *Vorschlag: ja, eigener .gp-Export als Kann.* → **Entschieden 15.10.2026:** MusicXML für v1.0, eigener .gp-Export Kann
- [x] **7.9 Projektformat:** → *Vorschlag: Projektordner mit `projekt.snotes` (XML) und Unterordner `audio/`.* → **Entschieden 15.10.2026:** Projektordner mit `projekt.snotes` und `audio/`
- [x] **7.10 Zeitbudget:** Wie viele Stunden pro Woche? → **Entschieden 15.10.2026:** 10,5 Std./Woche realistisch
- [x] **7.11 Echtzeit-Transkription** (Noten erscheinen schon beim Spielen)? → *Vorschlag: nicht in v1.0; die Transkription läuft nach der Aufnahme.* → **Entschieden 15.10.2026:** nicht in v1.0
- [x] **7.12 Spieltechniken:** Bends, Vibrato, Slides, Hammer-on/Pull-off als Zeichen in Noten/Tab? → *Vorschlag: Bends und Slides als Soll in v1.0 (Basic Pitch liefert Pitch-Bend-Daten), der Rest Kann.* → **Entschieden 15.10.2026:** Bends und Slides Soll, Rest Kann
- [x] **7.13 Lizenz und Sichtbarkeit:** AGPLv3 und öffentliches Repository wie Drum Programmer? → *Vorschlag: ja.* → **Entschieden 15.10.2026:** AGPLv3, öffentliches Repository

---

## 8. Out of Scope (vorerst)

- Plugin-Hosting und Plugin-Version (siehe 7.1)
- Audio-Schnitt, Mixing, Effekte über Lautstärke/Stumm/Solo hinaus
- Vollständiger Noteneditor (siehe 7.7)
- Training eigener Modelle; Cloud-Dienste
- Echtzeit-Transkription (siehe 7.11)
- macOS (technisch über JUCE möglich, später)

---

*Dieses Dokument ist eine lebende Planungsgrundlage. Nach Klärung von Abschnitt 7 folgt das Pflichtenheft mit Anforderungs-IDs, Arbeitspaketen und Meilensteinen.*
