# Pflichtenheft: Scratch Notes v1.0

Stand: 15.10.2026 · Entwurf v0.1 · Alex

Scratch Notes v1.0 wird als JUCE/C++-Standalone für Linux und Windows in 12 Arbeitspaketen mit rund 244 Stunden (293 Stunden inkl. 20 % Puffer) umgesetzt. Bei 10,5 Stunden pro Woche (Lastenheft 7.10) und Start am 19.10.2026 ist der MVP etwa Mitte März 2027 und Version 1.0 etwa Anfang Mai 2027 fertig. Grundlage sind das Lastenheft v0.2 (15.10.2026), der Python-PoC zur Transkription und das Projekt Drum Programmer, aus dem Gerüst und Audio-Module übernommen werden.

## 1 Zielbestimmung

Scratch Notes nimmt Improvisationen auf E-Gitarre und E-Bass auf, mit DI- und bearbeitetem Signal je Stimme, auch per Overdub, und schreibt sie danach als Noten und Tabulatur auf. Prioritäten: **Muss (MVP)** = Teil von Meilenstein M3, **Muss (v1.0)** = Pflicht für Release 1.0, **Soll** = geplant für v1.0, darf bei Zeitmangel verschoben werden, **Kann** = spätere Version.

### 1.1 Musskriterien MVP (M3)

- Stimmen mit 1 oder 2 Kanälen (DI + bearbeitet) anlegen, Eingängen zuordnen und aufnehmen
- Overdub: vorhandene Stimmen hören und eine neue aufnehmen, mit Latenzausgleich
- Mehrere Stimmen gleichzeitig aufnehmen (Proberaum)
- Roundtrip-Latenz ≤ 10 ms auf der Referenz-Hardware, gemessen und in der Statusleiste angezeigt
- Transkription der DI-Spur: Töne, gemeinsames Beat-Raster mit Tempokurve, Taktart-Vorschläge mit Konfidenz, Aufräumen, Quantisierung relativ zu den Beats
- Instrument-Profile Gitarre 6 (E), Gitarre 7 (G#), Bass 4 (E), Bass 5 (G#)
- Tabulatur mit Saite und Bund
- Anzeige von Noten und Tabulatur, Partitur aller Stimmen
- Export MIDI (Typ 1, Tempo-Map) und MusicXML (mit Tab)
- Projektordner speichern und laden, Undo/Redo
- Linux mit ALSA und JACK

### 1.2 Musskriterien Version 1.0

- Korrektur: Taktart-Deutung wählen, Beat-Raster bearbeiten und durch Mitklopfen setzen, Noten korrigieren, danach neu quantisieren
- Mitlaufende Position in den Noten, Nachspielen der Transkription mit A/B-Vergleich
- Windows mit ASIO (WASAPI als Fallback)
- WAV-Import als Take

### 1.3 Sollkriterien

- Bends und Slides als Zeichen in Noten und Tab
- Software-Monitoring je Stimme
- Optionaler Klick
- Notenbild als PDF/SVG exportieren

### 1.4 Kannkriterien (spätere Versionen)

- Eigener Guitar-Pro-Export (.gp)
- Vibrato, Hammer-on/Pull-off, Palm Mute
- Weitere Instrumente und Stimmungen (frei definierbar), Klavier
- Zweistimmige Notation innerhalb einer Stimme (Orgelpunkt + Melodie)
- Echtzeit-Transkription während der Aufnahme
- macOS

### 1.5 Abgrenzungskriterien

- Kein Plugin-Hosting, keine Plugin-Version
- Kein Audio-Schnitt, keine Effekte, kein Mixer über Lautstärke/Stumm/Solo hinaus
- Kein vollständiger Noteneditor
- Kein Training eigener Modelle, keine Cloud-Dienste

## 2 Produkteinsatz und Produktumgebung

Zielgruppe sind Gitarristen und Bassisten, die Ideen schnell festhalten und als Noten oder Tabulatur weiterverwenden wollen. Einzelplatz-Programm ohne Netzwerkzugriff.

| Bereich | Festlegung |
| --- | --- |
| Betriebssysteme | Linux (primär, Linux Mint), Windows 10/11 64 Bit |
| Framework | JUCE 8 (AGPLv3), C++20 |
| Build | CMake-Presets, Ninja, CLion; CI über GitHub Actions (ubuntu-latest, windows-latest mit MSVC) |
| Audio-Treiber | Linux: ALSA, JACK (auch PipeWire-JACK); Windows: ASIO (Steinberg-SDK unter GPLv3, von CMake geladen), WASAPI |
| Bibliotheken | ONNX Runtime (MIT, Binärpaket von CMake geladen), Basic-Pitch-Modell (Apache 2.0, eingebaut), Verovio (LGPLv3), GoogleTest/GoogleMock (BSD) |
| Referenz-Hardware | Zuhause: Steinberg UR44C unter Linux, Pedalboard in Line-In (bearbeitetes Signal) plus DI. Zweiter Gitarrist zuhause: Focusrite Clarett 2Pre USB unter Windows 11. Proberaum: SSL 12 unter Windows 11 (vermutlich), zwei Stacks, Signal per DI-Box gesplittet. 48 kHz |
| Dateiformate | Aufnahmen: WAV 32 Bit float; Import: WAV 16/24/32 Bit; Export: MIDI SMF Typ 1, MusicXML 4.0; Projekt: Ordner mit `projekt.snotes` (XML) |
| Lizenz des Produkts | AGPLv3, Quellcode öffentlich |

## 3 Systemarchitektur

Vier Threads mit klaren Grenzen: Der **Audio-Thread** liest Eingänge, schreibt sie in lock-freie Ring-Puffer und mischt die Wiedergabe aus vorgeladenen Puffern. Der **Schreib-Thread** holt die Eingangsdaten aus den Ring-Puffern und schreibt WAV-Dateien. Der **Transkriptions-Thread** arbeitet offline auf den fertigen Dateien. Der **GUI-Thread** besitzt das Modell. Zwischen GUI und Audio laufen wie bei Drum Programmer unveränderliche Snapshots (`SnapshotExchange`) und Event-Queues (`SpscQueue`).

| Modul (Quellordner) | Verantwortung | Herkunft |
| --- | --- | --- |
| `model/` | Projekt, Stimmen, Takes, Beat-Raster, Noten als ValueTree; Undo; Snapshots | Muster aus DP |
| `engine/` | Aufnahme in Ring-Puffer, Wiedergabe der Stimmen, Monitoring, Mix, Klick, Transport | `SpscQueue`, `SnapshotExchange`, `Clock`, `Metronome`, `BackingTrackPlayer`, `Resampler` aus DP |
| `audio/` | Treiberwahl, Geräteausfall, Ein- und Ausgangs-Routing | `DriverChoice`, `DeviceRecovery` aus DP, um Eingänge erweitert |
| `io/` | Projektordner, WAV-Schreiber, MIDI, MusicXML, Einstellungen | `MidiFileWriter`, `SettingsStore`, `StdFileSystem`, `ProjectRepository` aus DP |
| `transcription/` | Tonerkennung, Beat-Tracking, Taktart, Aufräumen, Quantisierung, Tabulatur; reines C++ ohne GUI | neu, Port des PoC |
| `notation/` | Partitur-Modell, MusicXML-Erzeugung, Rendern mit Verovio zu SVG | neu |
| `ui/` | Presenter (testbar), Spurenansicht, Notenansicht, Dialoge | Muster aus DP |
| `app/` | JUCE-Komponenten (Humble Objects), Composition Root | Muster aus DP |
| `i18n/` | Deutsch und Englisch | aus DP |
| `tools/` | Kommandozeilen-Werkzeug `scratchnotes-transcribe` für Tests und Feintuning ohne GUI | neu |

Das Kommandozeilen-Werkzeug ruft dieselbe `transcription/`-Bibliothek auf wie das Programm. Damit lassen sich Alex' vorhandene DI-Aufnahmen (Reaper-Projekte) in Serie transkribieren und die Schwellwerte abstimmen.

## 4 Datenmodell und Projektdatei

Audio-Positionen werden in Samples der Aufnahme gespeichert, musikalische Positionen in Ticks mit 960 PPQ relativ zum Beat-Raster (wie Drum Programmer). Erkannte Rohdaten, Quantisierung und Korrekturen des Nutzers werden getrennt gespeichert, damit eine erneute Quantisierung (z. B. nach Änderung des Beat-Rasters) die Korrekturen nicht verliert.

| Entität | Wichtige Attribute | Bemerkung |
| --- | --- | --- |
| Project | name, sampleRate, voices\[\], beatGrid, meterReadings\[\], chosenMeter, mix | Wurzelobjekt, `projekt.snotes` |
| Voice | id, name, profileId, inputDi, inputWet (optional), gain, mute, solo, monitor, takes\[\], activeTakeId | Stimme = ein Instrument |
| Take | id, diFile, wetFile (optional), startSample, latencyOffsetSamples, recordedAt | Dateien relativ in `audio/` |
| InstrumentProfile | id, name, strings\[\] (MIDI-Noten tief → hoch), frets, clef | MVP: guitar6-e, guitar7-gs, bass4-e, bass5-gs |
| BeatGrid | beats\[\] (Sample-Positionen), downbeatPhase, source (auto/edited) | gemeinsam für alle Stimmen |
| MeterReading | numerator, confidence, phase | Vorschläge der Analyse |
| DetectedNote | voiceId, startSample, endSample, midiPitch, amplitude, pitchBend\[\] | Rohdaten der Tonerkennung, unveränderlich |
| ScoreEvent | voiceId, startTick, durationTicks, pitches\[\], string/fret je Ton, grace, technique | Ergebnis von Aufräumen, Quantisierung und Tabulatur |
| UserEdit | voiceId, Ziel (Position oder DetectedNote), Änderung | wird nach jeder Neuberechnung wieder angewendet |

Das Modell ist ein JUCE `ValueTree` mit `UndoManager`; `projekt.snotes` ist XML mit `formatVersion="1"`. Die WAV-Dateien werden fortlaufend geschrieben und der Header regelmäßig aktualisiert, damit ein Take auch nach einem Absturz lesbar bleibt.

## 5 Funktionale Anforderungen

LH = Abschnitt im Lastenheft.

### 5.1 Projektverwaltung (LH 3.6)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-PJ-01 | Neues Projekt als Ordner mit `projekt.snotes` und `audio/` anlegen | Muss (MVP) |
| F-PJ-02 | Projekt öffnen, speichern, speichern unter; Nachfrage bei ungespeicherten Änderungen | Muss (MVP) |
| F-PJ-03 | Fehlende Audio-Dateien beim Laden melden, Projekt lädt trotzdem | Muss (MVP) |
| F-PJ-04 | Undo/Redo für alle Änderungen am Modell; ein Take ist ein Undo-Schritt | Muss (MVP) |
| F-PJ-05 | WAV-Datei als Take einer Stimme importieren (DI und optional bearbeitet) | Muss (v1.0) |

### 5.2 Aufnahme (LH 3.1)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-AU-01 | Stimme anlegen mit Name, Instrument-Profil, DI-Eingang und optional Eingang für das bearbeitete Signal | Muss (MVP) |
| F-AU-02 | Aufnahme aller scharfgeschalteten Stimmen als WAV 32 Bit float, ohne Allokation, Lock oder Dateizugriff im Audio-Thread | Muss (MVP) |
| F-AU-03 | Pegelanzeige je Eingang mit Übersteuerungs-Warnung | Muss (MVP) |
| F-AU-04 | Jeder Durchgang ergibt einen Take; Takes einer Stimme auflisten, aktiven wählen, löschen | Muss (MVP) |
| F-AU-05 | Mehrere Stimmen gleichzeitig aufnehmen (Proberaum: zwei Gitarristen) | Muss (MVP) |
| F-AU-06 | Ein Take bleibt nach Absturz oder Geräteausfall bis zur letzten geschriebenen Sekunde erhalten | Muss (v1.0) |

### 5.3 Wiedergabe und Overdub (LH 3.2)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-WI-01 | Transport: Play, Stop, Rec, Position, Rücksprung an den Anfang | Muss (MVP) |
| F-WI-02 | Wiedergabe der aktiven Takes, bearbeitetes Signal falls vorhanden, sonst DI | Muss (MVP) |
| F-WI-03 | Lautstärke, Stumm, Solo je Stimme | Muss (MVP) |
| F-WI-04 | Overdub: Wiedergabe und Aufnahme gleichzeitig, sample-genau synchron | Muss (MVP) |
| F-WI-05 | Latenzausgleich: neue Takes um die gemeldete Ein- plus Ausgangslatenz und einen einstellbaren Versatz verschieben | Muss (MVP) |
| F-WI-06 | Software-Monitoring des Eingangs je Stimme ein/aus | Soll |
| F-WI-07 | Optionaler Klick mit festem Tempo, Standard aus | Soll |

### 5.4 Transkription (LH 3.3)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-TX-01 | Transkription im Hintergrund mit Fortschritt und Abbruch, automatisch nach der Aufnahme oder per Befehl | Muss (MVP) |
| F-TX-02 | Tonerkennung aus der DI-Spur mit Basic Pitch (ONNX Runtime), Pegel-Gate gegen Geisternoten | Muss (MVP) |
| F-TX-03 | Bass-Profile: Analyse eine Oktave höher gerechnet, damit Töne unter A0 (G#0) und am unteren Modellrand erkannt werden | Muss (MVP) |
| F-TX-04 | Onset-Stärke aus allen Spuren, Beat-Tracking mit dynamischer Programmierung; metrische Ebene global festlegen, dann ±40 % lokale Drift | Muss (MVP) |
| F-TX-05 | Ein gemeinsames Beat-Raster mit Tempokurve für alle Stimmen | Muss (MVP) |
| F-TX-06 | Taktart-Vorschläge 3/4 bis 7/4 mit Konfidenz und Phase; Plausibilitätsprüfung über wiederkehrende Phrasen | Muss (MVP) |
| F-TX-07 | Aufräum-Regeln riff / melody, automatisch gewählt (Anteil Akkordanschläge ≥ 50 % → riff), wie im PoC | Muss (MVP) |
| F-TX-08 | Tonumfang auf das Instrument-Profil begrenzen | Muss (MVP) |
| F-TX-09 | Quantisierung relativ zu den Beats, je Beat bestes Raster aus Viertel / Achtel / Triole / Sechzehntel mit Komplexitätskosten | Muss (MVP) |
| F-TX-10 | Gleiche Eingabe und Einstellungen ergeben das gleiche Ergebnis (deterministisch) | Muss (MVP) |
| F-TX-11 | Bends und Slides aus den Pitch-Bend-Daten erkennen | Soll |

### 5.5 Tabulatur (LH 3.3)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-TB-01 | Saite und Bund je Ton nach Instrument-Profil zuordnen | Muss (MVP) |
| F-TB-02 | Griffweg optimieren (dynamische Programmierung über Lagenwechsel, spielbare Akkordgriffe) | Muss (MVP) |
| F-TB-03 | Saite einer Note von Hand ändern, Rest des Griffwegs passt sich an | Muss (v1.0) |

### 5.6 Anzeige (LH 3.4)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-NA-01 | Noten (oktavierter Violinschlüssel bzw. Bassschlüssel 8vb) und Tabulatur je Stimme, gerendert mit Verovio | Muss (MVP) |
| F-NA-02 | Partitur-Ansicht aller Stimmen untereinander | Muss (MVP) |
| F-NA-03 | Spurenansicht mit Wellenform und Beat-Raster | Muss (MVP) |
| F-NA-04 | Mitlaufende Position in den Noten beim Abspielen | Muss (v1.0) |
| F-NA-05 | Transkription nachspielen (Synthese), A/B-Vergleich mit dem Original | Muss (v1.0) |

### 5.7 Korrektur (LH 3.4)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-KO-01 | Taktart-Deutung per Klick wählen | Muss (v1.0) |
| F-KO-02 | Beats verschieben, einfügen, löschen; Downbeat setzen | Muss (v1.0) |
| F-KO-03 | Beat-Raster durch Mitklopfen (Taste) während der Wiedergabe neu setzen | Muss (v1.0) |
| F-KO-04 | Note löschen, Tonhöhe und Dauer ändern | Muss (v1.0) |
| F-KO-05 | Nach Änderung des Rasters neu quantisieren, Nutzer-Korrekturen bleiben erhalten | Muss (v1.0) |

### 5.8 Export (LH 3.5)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-EX-01 | MIDI SMF Typ 1: eine Spur je Stimme, Tempo-Event auf jedem Beat, Taktart; läuft in einer DAW synchron zur Aufnahme | Muss (MVP) |
| F-EX-02 | MusicXML mit Tabulatur (Saite/Bund, Stimmung), öffnet in MuseScore und Guitar Pro 8 | Muss (MVP) |
| F-EX-03 | Notenbild als PDF oder SVG | Soll |
| F-EX-04 | Eigener Guitar-Pro-Export (.gp) | Kann |

### 5.9 Audio-Treiber und Einstellungen (LH 3.7)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-AO-01 | Audio-Einstellungen: Treiber, Gerät, Samplerate, Puffer, Ein- und Ausgänge | Muss (MVP) |
| F-AO-02 | Linux: ALSA und JACK | Muss (MVP) |
| F-AO-03 | Windows: ASIO, WASAPI als Fallback | Muss (v1.0) |
| F-AO-04 | Geräteausfall abfangen, laufende Aufnahme sauber beenden, Gerät bei Rückkehr wieder öffnen | Muss (v1.0) |

### 5.10 Sprache und Farbschema (aus DP übernommen)

| ID | Anforderung | Priorität |
| --- | --- | --- |
| F-UI-01 | Oberfläche auf Deutsch und Englisch, einschließlich der JUCE-eigenen Dialoge (Audio-Einstellungen, Dateiauswahl) | Muss (MVP) |
| F-UI-02 | Sprache im Menü Hilfe wählbar; Default ist die Sprache des Betriebssystems, sonst Englisch; die Wahl wird gespeichert und gilt ab dem nächsten Start | Muss (MVP) |
| F-UI-03 | Erweiterbar: eine weitere Sprache ist eine Übersetzungsdatei ohne Code-Änderung; fehlende Übersetzungen erscheinen englisch | Muss (MVP) |
| F-UI-04 | Dunkles und helles Farbschema; Default ist der Hell-/Dunkelmodus des Betriebssystems und folgt dessen Wechsel | Muss (MVP) |
| F-UI-05 | Farbschema im Menü Hilfe wählbar (Systemeinstellung, Dunkel, Hell); die Wahl wird gespeichert und gilt sofort, ohne Neustart | Muss (MVP) |

## 6 Benutzeroberfläche

Ein Hauptfenster mit drei Bereichen: oben Transportleiste, links die **Spurenansicht** (Stimmen mit Eingängen, Rec-Taste, Pegel, Lautstärke, Wellenform der Takes und Beat-Raster darüber), rechts bzw. umschaltbar die **Notenansicht** (Partitur mit Noten und Tab, Taktart-Auswahl oben). Unten die Statusleiste mit Treiber, Puffer und Latenz wie bei Drum Programmer. Ein GUI-Entwurf entsteht in AP0 und wird mit Alex abgestimmt.

## 7 Qualitätsanforderungen

| ID | Merkmal | Messbares Kriterium | Priorität |
| --- | --- | --- | --- |
| Q-01 | Roundtrip-Latenz | ≤ 10 ms, Ziel 5 ms; gemessen mit Loopback-Kabel und `jack_iodelay` bzw. eingebauter Messung. 5 ms erfordern 64 Samples bei 48 kHz | Muss (MVP) |
| Q-02 | Puffer einstellbar | 32 bis 2048 Samples; Statusleiste zeigt Ein-, Ausgangs- und Roundtrip-Latenz | Muss (MVP) |
| Q-03 | Dropout-Freiheit | 30 Minuten Overdub mit 4 Stimmen à 2 Kanälen bei Puffer 128 ohne XRun | Muss (v1.0) |
| Q-04 | Echtzeit-Sicherheit | Im Audio-Callback keine Allokation, keine Locks, kein Dateizugriff, kein Logging | Muss (MVP) |
| Q-05 | Synchronität | Overdub-Take liegt nach Latenzausgleich ≤ 1 ms neben der Referenz (Loopback-Test) | Muss (MVP) |
| Q-06 | Transkriptionszeit | ≤ Aufnahmedauer für eine Stimme auf dem Entwicklungsrechner | Muss (MVP) |
| Q-07 | Transkriptionsqualität | Auf den drei PoC-Aufnahmen mindestens die PoC-Werte: Einsätze F1 ≥ 0,72 / 0,84, Tonklassen ≥ 0,86 / 0,82 (Riff / Improvisation DI), Beats F ≥ 0,90 DI vs. Distortion | Muss (MVP) |
| Q-08 | Tiefe Töne | Testaufnahmen 7-Saiter und 5-Saiter-Bass: offene tiefste Saite wird erkannt (Einsatz und Tonhöhe) | Muss (MVP) |
| Q-09 | Robustheit | Kein Datenverlust bei Absturz während der Aufnahme, fehlenden Dateien oder Gerätewechsel | Muss (v1.0) |
| Q-10 | Datensicherheit | Speichern über temporäre Datei und Umbenennen | Muss (MVP) |
| Q-11 | Bedienbarkeit | Vom Programmstart bis zur laufenden Aufnahme einer Stimme in ≤ 30 s, ohne Anleitung | Muss (v1.0) |
| Q-12 | Lizenz | Nur Open-Source-Bibliotheken; ASIO-SDK und ONNX Runtime beim Bauen geladen; Lizenzhinweise im Über-Dialog | Muss (MVP) |
| Q-13 | Portabilität | Gleicher Quellcode für Linux und Windows, plattformspezifischer Code in getrennten Dateien | Muss (MVP) |

### Entwicklungsvorgaben

Die Vorgaben E-01 bis E-09 gelten unverändert wie im Pflichtenheft von Drum Programmer (TDD, 100 % Zeilen- und Branch-Abdeckung mit begründeten Ausnahmen, Humble Object, Constructor Injection, eine Composition Root, Abstraktionen nach außen, statische DI im Echtzeit-Pfad, Clean Code mit clang-format/clang-tidy und Warnungen als Fehler, GoogleTest und Sanitizer in CI). Dazu kommen:

| ID | Vorgabe | Umsetzung | Nachweis |
| --- | --- | --- | --- |
| E-10 | Regressions-Tests der Transkription | Referenz-Aufnahmen mit erwarteten Kennzahlen (Q-07, Q-08) als CTest; kleine Ausschnitte im Repository, vollständige Aufnahmen per Download oder lokal | CI-Job „Transkription“, Kennzahlen als Artefakt |
| E-11 | Referenz-Transkriptionen | 2–3 Aufnahmen von Hand korrigiert (MusicXML) als echte Genauigkeitsmessung, statt nur Selbstkontrolle | ab AP5 |
| E-12 | Numerische Gleichheit zum PoC | Für Teilschritte (Onset-Stärke, Beats, Notenbildung aus Basic-Pitch-Ausgabe) Vergleich mit den Zwischenwerten des Python-Skripts innerhalb einer Toleranz | Unit-Tests mit exportierten Python-Zwischenwerten |

## 8 Projektplanung: Arbeitspakete

12 Arbeitspakete mit zusammen 244 Stunden, mit 20 % Puffer 293 Stunden. Grundlage: 10,5 Stunden pro Woche, Start 19.10.2026. Durch die Übernahme aus Drum Programmer sind Setup und Treiber-Anbindung deutlich kleiner als dort.

| AP | Inhalt | Aufgaben | Anforderungen | Std. | Std. inkl. Puffer |
| --- | --- | --- | --- | --- | --- |
| AP0 | Setup | Repo, Gerüst aus DP (CMake-Presets, Submodule, CI, Coverage, clang-tidy, ASIO), ONNX Runtime und Verovio in CMake, Audio-Gerät mit Ein- und Ausgängen, GUI-Entwurf | F-AO-01, F-AO-02, F-UI-01 bis 05, Q-12, Q-13, E-01 bis E-09 | 12 | 14,4 |
| AP1 | Datenmodell und Projektordner | ValueTree-Modell, Snapshots, Projektordner speichern/laden, Undo | F-PJ-01 bis 04, Q-10 | 14 | 16,8 |
| AP2 | Aufnahme-Engine | Eingangs-Routing, Ring-Puffer, Schreib-Thread, WAV mit fortlaufendem Header, Pegel, Takes, gleichzeitige Stimmen, Spurenansicht | F-AU-01 bis 05, Q-04 | 22 | 26,4 |
| AP3 | Wiedergabe und Overdub | Wiedergabe der Stimmen (aus `BackingTrackPlayer`), Mix, Latenzausgleich, eingebaute Latenzmessung, Monitoring, Klick | F-WI-01 bis 07, Q-01, Q-02, Q-05 | 22 | 26,4 |
| AP4 | Tonerkennung | ONNX Runtime anbinden, Basic-Pitch-Fenster und Notenbildung portieren, Oktav-Verschiebung für Bass, Pegel-Gate, Kommandozeilen-Werkzeug | F-TX-01 bis 03, F-TX-08, E-12 | 24 | 28,8 |
| AP5 | Rhythmus und Aufräumen | Onset-Stärke, Beat-Tracker, Tempokurve, Taktart, Aufräum-Regeln, Quantisierung, Regressions-Tests gegen PoC | F-TX-04 bis 07, F-TX-09, F-TX-10, Q-06, Q-07, E-10, E-11 | 36 | 43,2 |
| AP6 | Tabulatur und Instrument-Profile | vier Profile, Saite/Bund, Griffweg-Optimierung, Testaufnahmen 7-Saiter und 5-Saiter | F-TB-01, F-TB-02, Q-08 | 14 | 16,8 |
| AP7 | Notenanzeige | Partitur-Modell, MusicXML intern, Verovio → SVG → JUCE, Partitur aller Stimmen | F-NA-01 bis 03 | 26 | 31,2 |
| AP8 | Export | MIDI Typ 1 mit Tempo-Map (`MidiFileWriter` erweitern), MusicXML mit Tab, Test in Reaper, MuseScore, Guitar Pro | F-EX-01, F-EX-02 | 10 | 12,0 |
| AP9 | Korrektur | Taktart wählen, Beat-Raster bearbeiten, Mitklopfen, Noten korrigieren, Neuberechnung mit Erhalt der Korrekturen, mitlaufende Position, Nachspielen | F-KO-01 bis 05, F-TB-03, F-NA-04, F-NA-05 | 30 | 36,0 |
| AP10 | Windows und Robustheit | WAV-Import, ASIO/WASAPI-Tests, Geräteausfall | F-AU-06, F-PJ-05, F-AO-03, F-AO-04, Q-09 | 14 | 16,8 |
| AP11 | Stabilisierung und Release | Bends/Slides (Soll), PDF-Export (Soll), Dauertest, Bugfixing, Release-Pakete (AppImage, Windows-Installer) | F-TX-11, F-EX-03, Q-03, Q-11 | 20 | 24,0 |
| | **Summe** | | | **244** | **292,8** |

**Reihenfolge.** AP0 bis AP8 bilden den MVP (180 Std. netto, 216 Std. mit Puffer). AP4 bis AP6 lassen sich vor AP2/AP3 ziehen, falls die Transkription zuerst erprobt werden soll: Sie brauchen nur WAV-Dateien und das Kommandozeilen-Werkzeug. Empfehlung: Reihenfolge wie oben, weil die Latenz-Architektur das größere Architektur-Risiko ist und das Aufnehmen früh im Alltag nutzbar wird.

## 9 Zeitplan und Meilensteine

Bei 10,5 Stunden pro Woche ist der MVP nach etwa 21 Wochen erreicht (ca. 15.3.2027), Version 1.0 nach etwa 28 Wochen (ca. 3.5.2027). Urlaube, Feiertage und parallele Arbeit an Drum Programmer sind nicht eingerechnet.

| Meilenstein | nach AP | Inhalt |
| --- | --- | --- |
| M0 Setup | AP0 | Programm startet, Eingänge sichtbar, CI für Linux und Windows grün |
| M1 Aufnahme | AP3 | Zwei Stimmen per Overdub aufnehmen, synchron abspielen, Latenz gemessen |
| M2 Transkription | AP6 | Kommandozeilen-Werkzeug liefert Noten und Tab, Q-07 und Q-08 erfüllt |
| M3 MVP | AP8 | Aufnehmen, transkribieren, anzeigen, exportieren in einem Programm |
| M4 Release 1.0 | AP11 | Korrektur, mehrstimmige Aufnahme, Windows mit ASIO, stabil |

## 10 Test- und Abnahmekonzept

### 10.1 Testarten

| Testart | Werkzeug | Umfang |
| --- | --- | --- |
| Unit-Tests | GoogleTest/GoogleMock, test-first, 100-%-Coverage-Gate | Modell, Routing, Ring-Puffer, Latenzausgleich, alle Transkriptions-Schritte, Tabulatur, MIDI/MusicXML |
| Offline-Render-Test | Test-Treiber ohne Audio-Gerät | Overdub-Synchronität sample-genau |
| Regressions-Tests Transkription | CTest mit Referenz-Aufnahmen | Q-07, Q-08 (E-10) |
| Vergleich mit PoC | exportierte Python-Zwischenwerte | E-12 |
| Thread-Prüfung | ThreadSanitizer | Audio-, Schreib-, Transkriptions- und GUI-Thread |
| Latenz-Messung | Loopback-Kabel, `jack_iodelay`, eingebaute Messung | Q-01, Q-02, Q-05 |
| Dauertest | 30 Minuten Overdub, XRun-Zähler | Q-03 |
| Import-Test | Reaper, MuseScore, Guitar Pro 8 | F-EX-01, F-EX-02 |

### 10.2 Abnahmeszenarien

| Meilenstein | Szenario |
| --- | --- |
| M0 Setup | Programm startet auf Linux, zeigt Ein- und Ausgänge des Interfaces, Testton hörbar; Windows-Build in CI grün |
| M1 Aufnahme | Gitarre mit DI und bearbeitetem Signal aufnehmen; danach Bass per Overdub zur Gitarre; beide synchron abspielen; Loopback-Test zeigt ≤ 1 ms Versatz; Roundtrip ≤ 10 ms in der Statusleiste und gemessen |
| M2 Transkription | `scratchnotes-transcribe` auf den drei PoC-Aufnahmen erreicht die PoC-Kennzahlen; auf je einer Aufnahme mit 7-Saiter und 5-Saiter-Bass wird die tiefste Saite erkannt; Tab ist spielbar |
| M3 MVP | Improvisation aufnehmen, Transkription startet automatisch, Noten und Tab beider Stimmen erscheinen; MIDI läuft in Reaper synchron zur Aufnahme; MusicXML öffnet in MuseScore und Guitar Pro mit Tab |
| M4 Release 1.0 | Taktart umstellen, ein Beat-Raster durch Mitklopfen korrigieren, Noten werden neu gesetzt und Korrekturen bleiben; zwei Stimmen gleichzeitig aufnehmen; Szenarien M1–M3 auf Windows mit ASIO bestanden; Dauertest ohne XRun |

## 11 Risiken

| Risiko | Wahrscheinlichkeit | Auswirkung | Gegenmaßnahme |
| --- | --- | --- | --- |
| Tiefe Töne (G#0, G#1, E1) werden schlecht erkannt | hoch | Bass- und 7-Saiter-Noten unbrauchbar | Oktav-Verschiebung für die Analyse (F-TX-03), Profil-Filter, früh mit echten Aufnahmen testen (AP4); Notlösung: einstimmiger Pitch-Tracker für Bass |
| C++-Port der Transkription weicht vom PoC ab | mittel | Qualität schlechter als erwartet | Zwischenwerte des Python-Skripts als Testdaten (E-12), Schritt für Schritt vergleichen |
| Taktart und Beats bei freier Improvisation unsicher | sicher | Notenbild in falscher Taktart | ist erwartet (PoC 6.4): mehrere Deutungen anbieten, Korrektur durch den Menschen (AP9) |
| 5 ms nicht erreichbar | mittel | Ziel verfehlt, 10 ms meist erreichbar | Puffer 64 bei 48 kHz oder 128 bei 96 kHz, JACK mit Echtzeit-Priorität, Direct Monitoring als Alternative |
| Verovio-Anzeige in JUCE langsam oder SVG-Darstellung unvollständig | mittel | Notenansicht ruckelt | Seitenweise rendern und cachen; Alternative: Verovio-Ausgabe als Bild über eine WebView |
| ONNX Runtime unter Windows/Linux im Build | niedrig | Build-Aufwand | offizielle Binärpakete, in AP0 schon in CI |
| Schwellwerte nur an einem Spieler abgestimmt | mittel | schlechter bei anderem Material | Alex' Reaper-DI-Aufnahmen in Serie über das Kommandozeilen-Werkzeug auswerten |
| Zeit neben Drum Programmer | mittel | Termine verschieben sich | Zeitbudget klären (Lastenheft 7.10), nach M1 Geschwindigkeit messen und neu planen |

## 12 Klärungsbedarf

| Nr. | Thema | Stand |
| --- | --- | --- |
| K1 | Zeitbudget pro Woche | entschieden 15.10.2026: 10,5 Std./Woche realistisch |
| K2 | Stimmung 5-Saiter-Bass G#0 C#1 F#1 B1 E2 | bestätigt 15.10.2026 |
| K3 | Referenz-Hardware | geklärt 15.10.2026, siehe Kapitel 2. Folge: Im Proberaum nehmen zwei Gitarristen gleichzeitig unter Windows auf; F-AU-05 und Windows/ASIO (AP10) sind deshalb früh wichtig. Windows läuft ab AP0 in CI; ein erster Test auf dem SSL 12 ist nach M1 geplant |
| K4 | Testaufnahmen 7-Saiter und 5-Saiter-Bass (DI, je 30–60 s) für AP4/AP6 | Alex nimmt später auf, gebraucht ab AP4 |

## Quellen

- Lastenheft Scratch Notes v0.2, Stand 15.10.2026
- Proof of Concept „Tempo, Taktart und Noten aus freier Improvisation“, Stand 15.10.2026 (`ZUSAMMENFASSUNG.md`, `rhythm_transcribe.py`)
- Drum Programmer, Pflichtenheft v1.0 und Quellcode (`github.com/ladych/DrumProgrammer`)
- Basic Pitch 0.4.0, `constants.py`: tiefster erkannter Ton A0 = 27,5 Hz
