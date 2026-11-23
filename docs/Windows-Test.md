# Testprotokoll Windows und Robustheit (AP10)

Zum Abhaken im Proberaum auf den beiden Windows-11-Rechnern: SSL 12 (zwei Gitarren über DI-Box gesplittet) und Clarett 2Pre USB. Getestet werden F-AO-03 (ASIO, WASAPI als Ersatz), F-AO-04 (Geräteausfall), F-AU-06 und Q-09 (kein Datenverlust), F-PJ-05 (Import) und nebenbei Q-01/Q-02/Q-05 (Latenz) und F-AU-05 (zwei Stimmen gleichzeitig).

**Programm:** In GitHub unter *Actions* den letzten grünen Lauf von `main` öffnen und das Artefakt **ScratchNotes-Windows** laden. Den Ordner entpacken und `Scratch Notes.exe` starten; es muss nichts installiert werden. Vor dem ersten Test den Treiber des Interfaces in der aktuellen Version installieren (SSL 12: SSL USB Control Panel, Clarett: Focusrite Control).

**Befunde:** Je Fehler ein GitHub-Issue mit der Nummer des Schritts (z. B. „W3.2: …“), Interface, Treiber, Samplerate, Buffer und einem Foto oder Screenshot der Statusleiste. Hilfreich ist auch die Log-Datei, falls vorhanden.

Ergebnis je Zeile: ✓ (wie erwartet), ✗ (Fehler, Issue anlegen) oder – (nicht getestet).

## W1 Treiberwahl (F-AO-03, F-AO-01)

| Nr. | Schritt | Erwartet | Ergebnis |
| --- | --- | --- | --- |
| 1.1 | Erster Start ohne gespeicherte Einstellungen: vorher `%APPDATA%\ScratchNotes\audio-device.xml` löschen, Interface angesteckt | Statusleiste zeigt **ASIO** mit dem Treiber des Interfaces und dessen Eingängen | |
| 1.2 | Audio → Audio-Einstellungen (Strg+,): Treiber, Samplerate 48 kHz, Buffer 128 wählen, Dialog schließen, Programm neu starten | Einstellungen bleiben erhalten | |
| 1.3 | Im Dialog den ASIO-Knopf „Control Panel“ öffnen und dort den Buffer ändern | Statusleiste übernimmt den neuen Buffer; Ton läuft weiter | |
| 1.4 | Treiber im Dialog auf **Windows Audio** (WASAPI) stellen, Interface als Ein- und Ausgang | Testton (Strg+T) ist zu hören, Pegel bewegen sich | |
| 1.5 | Wie 1.4 mit **Windows Audio (Exclusive Mode)** | wie 1.4, kleinere Latenz in der Statusleiste | |
| 1.6 | ASIO-Treiber eines anderen Programms belegt (z. B. Reaper mit ASIO offen lassen), dann Scratch Notes starten | Programm startet trotzdem, fällt auf WASAPI zurück, Statusleiste nennt das ersetzte Gerät | |

## W2 Latenz (Q-01, Q-02, Q-05)

| Nr. | Schritt | Erwartet | Ergebnis |
| --- | --- | --- | --- |
| 2.1 | ASIO, 48 kHz, Buffer 64 / 128 / 256: Roundtrip aus der Statusleiste notieren | 128: ≤ 10 ms, 64: um 5 ms | 64: ___ ms, 128: ___ ms, 256: ___ ms |
| 2.2 | Loopback-Kabel Ausgang 1 → Eingang 1, Audio → Latenzausgleich… → Messen | gemessener Roundtrip etwas über dem der Statusleiste | ___ ms |
| 2.3 | WASAPI (geteilt und exklusiv), Buffer so klein wie möglich, Roundtrip notieren | zum Vergleich; Q-01 gilt für ASIO | geteilt ___ ms, exklusiv ___ ms |
| 2.4 | Testton 5 Minuten bei ASIO/128 laufen lassen | keine Knackser, XRuns in der Statusleiste bleiben 0 | |
| 2.5 | Overdub: Klick (Strg+K) aufnehmen, dann über das Loopback-Kabel eine zweite Stimme dazu aufnehmen; beide Takes in den Spuren vergleichen (oder exportieren und in Reaper messen) | Versatz ≤ 1 ms | ___ ms |

## W3 Aufnahme mit zwei Stimmen (F-AU-05)

| Nr. | Schritt | Erwartet | Ergebnis |
| --- | --- | --- | --- |
| 3.1 | SSL 12: zwei Stimmen anlegen, je DI-Eingang und bearbeiteter Eingang über die DI-Box, beide scharf (R), 2 Minuten zusammen spielen | vier Spuren mit Wellenform, Pegel ohne Übersteuern | |
| 3.2 | Aufnahme stoppen | Noten beider Stimmen werden im Hintergrund erkannt, ein gemeinsames Beat-Raster entsteht | |
| 3.3 | Clarett 2Pre: eine Stimme mit DI und bearbeitetem Signal aufnehmen | wie 3.1 | |

## W4 Geräteausfall (F-AO-04, Q-09)

| Nr. | Schritt | Erwartet | Ergebnis |
| --- | --- | --- | --- |
| 4.1 | Ohne Aufnahme: USB-Kabel des Interfaces abziehen | nach etwa 3 s meldet die Statusleiste „Audio-Gerät antwortet nicht“, dann läuft das Programm über WASAPI bzw. die Onboard-Karte weiter | |
| 4.2 | Interface wieder anstecken | Programm kehrt von selbst zum ASIO-Gerät zurück, Statusleiste zeigt es wieder | |
| 4.3 | Während einer Aufnahme (nach etwa 30 s) das USB-Kabel abziehen | Aufnahme endet von selbst mit der Meldung „Das Audio-Gerät ist ausgefallen oder wurde gewechselt…“; der Take ist da und enthält die gespielten 30 s (bis auf höchstens die letzte Sekunde) | |
| 4.4 | Während einer Aufnahme in den Audio-Einstellungen den Buffer ändern | Aufnahme endet mit derselben Meldung, der Take bleibt | |
| 4.5 | Projekt speichern, schließen, wieder öffnen | Takes aus 4.3 und 4.4 sind da und spielen ab | |

## W5 Absturz (F-AU-06, Q-09)

| Nr. | Schritt | Erwartet | Ergebnis |
| --- | --- | --- | --- |
| 5.1 | Gespeichertes Projekt, Aufnahme starten, nach etwa 30 s das Programm im Task-Manager beenden („Task beenden“) | im Projektordner liegen `projekt.snotes.recovery` und die WAV-Dateien des Takes | |
| 5.2 | Programm wieder starten | Frage „Wiederherstellen nach Absturz“; nach **Wiederherstellen** ist der Take aus 5.1 da (fast die ganzen 30 s), das Projekt ist als ungespeichert markiert | |
| 5.3 | Speichern, Programm normal beenden und neu starten | keine Frage mehr, `projekt.snotes.recovery` ist weg | |
| 5.4 | Eine Note korrigieren oder die Lautstärke ändern, 2 s warten, Programm im Task-Manager beenden, neu starten, **Verwerfen** wählen | Projekt ist wie zuletzt gespeichert, `projekt.snotes.recovery` ist weg | |

## W6 Import (F-PJ-05)

| Nr. | Schritt | Erwartet | Ergebnis |
| --- | --- | --- | --- |
| 6.1 | Datei → Audio als neue Stimme importieren…: eine DI-Spur aus einem Reaper-Projekt (z. B. 44,1 kHz, 24 Bit) | neue Stimme mit dem Dateinamen, Take spielt ab, Noten werden erkannt | |
| 6.2 | „+“ einer Stimme: DI- und bearbeitete Datei zusammen wählen | ein Take mit beiden Spuren, DI richtig zugeordnet | |
| 6.3 | Stereo-Datei mit anderer Samplerate als die Takes des Projekts importieren | Take hat die richtige Länge und Tonhöhe (nicht zu schnell oder zu langsam) | |
| 6.4 | Bearbeiten → Rückgängig | importierter Take (bzw. die neue Stimme) ist wieder weg | |

## Notizen

Interface / Treiberversion: ____________________ Windows-Build: ____________ Programm-Version (Hilfe → Info): ____________
