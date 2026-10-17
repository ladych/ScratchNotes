# Quellen des GUI-Entwurfs

`../GUI-Entwurf_v0.1.png` (dunkel) und `../GUI-Entwurf_v0.1_hell.png` (hell) entstehen aus diesen Dateien:

1. `python3 score.py 2600 4000 40` (braucht `pip install verovio`) schreibt `riff.musicxml` und `score.svg`, das Notenbeispiel mit drei Stimmen, gerendert mit derselben Verovio-Version wie das Programm.
2. In `entwurf.html` den Platzhalter `SCORE_SVG` durch den Inhalt von `score.svg` ersetzen, das öffnende `<svg` durch `<svg class="score"`.
3. Mit Chromium als Bild speichern, 1960 Pixel breit, hell mit `entwurf.html?light`.
