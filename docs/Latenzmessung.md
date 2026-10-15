# Latenzmessung (Q-01, Q-02)

Ziel nach Q-01: Buffer 128 Samples bei 48 kHz läuft ohne Dropouts, Roundtrip-Latenz ≤ 10 ms, Ziel 5 ms. Roundtrip heißt hier: vom Eingang des Interfaces (Gitarre am DI-Eingang) bis zum Ausgang (Kopfhörer), also das, was beim Overdub mit Software-Monitoring zu hören ist.

## Was die Statusleiste anzeigt

Die Statusleiste unten im Hauptfenster zeigt Treiber, Gerät, Samplerate, Buffer, die Zahl der aktiven Ein- und Ausgänge und:

- **Roundtrip**: Eingangs- plus Ausgangslatenz, mit beiden Teilwerten in Klammern.
- **Eingang** und **Ausgang**: die Latenzen, die der Treiber meldet (bei JACK die Port-Latenz, also alle Perioden). Meldet der Treiber nichts, steht dort ein Buffer.
- „über 10 ms“ hinter dem Roundtrip, wenn Q-01 verfehlt ist.
- „Kein Eingang, Aufnahme nicht möglich“, wenn das Gerät ohne Eingänge geöffnet ist.

Das sind berechnete Werte ohne Wandler-Latenz des Interfaces. Die echte Latenz misst man so:

## 1. Roundtrip mit jack_iodelay (Linux)

1. Ausgang des Interfaces per Kabel auf einen Eingang legen (Loopback), z. B. UR44C Line Out 1 → In 1. Pegel niedrig halten.
2. JACK mit 48 kHz, 128 Frames/Periode, 2 Perioden starten, z. B. `jackd -d alsa -d hw:UR44C -r 48000 -p 128 -n 2`.
3. `jack_iodelay` in einem Terminal starten. Es läuft weiter und meldet „Signal below threshold…“, solange noch nichts verbunden ist.
4. In einem **zweiten** Terminal verbinden (oder in QjackCtl unter „Verbindungen“):
   `jack_connect jack_delay:out system:playback_1` und `jack_connect system:capture_1 jack_delay:in`.
   Die genauen Portnamen zeigt `jack_lsp`. Kommt weiter „below threshold“, ist das Loopback-Kabel nicht am richtigen Eingang oder der Eingangspegel zu niedrig.
5. `jack_iodelay` gibt die gemessene Roundtrip-Latenz in Frames und ms aus, abzüglich der Systemlatenz die „extra loopback latency“ (Wandler und USB).

Der gemessene Wert sollte ungefähr dem Roundtrip der Statusleiste plus der „extra loopback latency“ entsprechen.

## 2. Roundtrip unter Windows (ASIO)

Unter Windows gibt es kein `jack_iodelay`. Bis zur eingebauten Messung (F-WI, AP3) misst man mit Reaper: Loopback-Kabel wie oben, auf einer Spur einen Klick abspielen und ihn über das Kabel auf einer zweiten Spur aufnehmen. Vorher in Reaper den Latenzausgleich der Aufnahme ausschalten; dann ist der Abstand der beiden Klicks die Roundtrip-Latenz.

## 3. Dropouts

Bei Buffer 128 einige Minuten mit Testton (Audio → Testton, Strg+T) laufen lassen und auf Knackser achten. JACK zählt XRuns (`jack_xrun` im Log oder in QjackCtl), die Statusleiste zeigt sie, wenn der Treiber sie zählt. Ab AP11 gibt es den 30-Minuten-Dauertest mit Overdub (Q-03).

## 4. Latenzausgleich beim Overdub (Q-05)

Ab AP3 verschiebt das Programm einen Overdub-Take um die gemessene Roundtrip-Latenz nach vorne, damit er zur gehörten Wiedergabe passt. Abnahme: Loopback-Test mit ≤ 1 ms Versatz zwischen Referenz und Take.
