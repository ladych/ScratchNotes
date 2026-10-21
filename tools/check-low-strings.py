#!/usr/bin/env python3
"""Checks that the open lowest string of the 7-string guitar and the 5-string bass is detected (Q-08).

Runs scratchnotes-transcribe on the DI takes in RECORDINGS/g#guitar (profile guitar7-gs, lowest string G#1)
and RECORDINGS/g#bass (bass5-gs, G#0) and compares the notes with pYIN as independent pitch, since there
is no hand-made score of these takes:

  found        notes on the pitch of the open lowest string
  agree        of these, the share whose pYIN pitch inside the note is the same (+-0.6 semitones)
  covered      of the frames where pYIN hears the open lowest string, the share a note of that pitch covers
  open in tab  of these notes, the share the tablature puts on the open lowest string

Q-08 asks that the open lowest string is detected: it holds for an instrument if its takes together cover
at least half of what pYIN hears there and the tab puts >= 0.8 of the notes on the open lowest string.
agree is reported only: pYIN follows one pitch, so under chords and held notes it hears another one.
Writes OUT/low-strings.json and fails if Q-08 does not hold.
Usage: check-low-strings.py CLI RECORDINGS OUT
Needs: pip install librosa soundfile
"""
import glob
import json
import os
import subprocess
import sys

import librosa
import numpy as np

SR = 22050
HOP = 256
INSTRUMENTS = {"g#guitar": ("guitar7-gs", 32, 7), "g#bass": ("bass5-gs", 20, 5)}
MIN_COVERED = 0.5
MIN_OPEN = 0.8


def read_notes(path):
    with open(path) as lines:
        next(lines)
        return [(float(start), float(end), int(pitch)) for start, end, pitch, _ in
                (line.rstrip("\n").split(",") for line in lines)]


def open_in_tab(path, pitch, strings):
    """How many notes of the pitch the events place on the open lowest string, of how many notes of it."""
    placed = total = 0
    with open(path) as lines:
        next(lines)
        for line in lines:
            fields = line.rstrip("\n").split(",")
            for note, place in zip(fields[4].split(), fields[6].split()):
                if int(note) == pitch:
                    total += 1
                    placed += place == f"{strings}:0"
    return placed, total


def check_take(path, pitch, notes):
    y, _ = librosa.load(path, sr=SR, mono=True)
    f0, voiced, _ = librosa.pyin(y, fmin=22.0, fmax=700.0, sr=SR, frame_length=4096, hop_length=HOP)
    times = librosa.times_like(f0, sr=SR, hop_length=HOP)
    midi = librosa.hz_to_midi(np.nan_to_num(f0, nan=1.0))
    found = [(start, end) for start, end, note in notes if note == pitch]
    agree = 0
    for start, end in found:
        inside = (times >= start + 0.03) & (times <= max(end, start + 0.06)) & voiced
        agree += inside.sum() >= 2 and abs(np.median(midi[inside]) - pitch) <= 0.6
    heard = voiced & (np.abs(midi - pitch) <= 0.5)
    covered = np.zeros_like(heard)
    for start, end in found:
        covered |= (times >= start) & (times <= end)
    return {"found": len(found), "agree": int(agree), "pyin_frames": int(heard.sum()),
            "covered_frames": int((heard & covered).sum())}


def main(cli, recordings, out):
    os.makedirs(out, exist_ok=True)
    report = {}
    passed = True
    for folder, (profile, pitch, strings) in INSTRUMENTS.items():
        takes = sorted(glob.glob(os.path.join(recordings, folder, "* DI.wav")))
        subprocess.run([cli, "--profile", profile, "--separate", "--out", out, *takes], check=True)
        total = {"found": 0, "agree": 0, "pyin_frames": 0, "covered_frames": 0, "open": 0, "in_tab": 0}
        for take in takes:
            stem = os.path.join(out, os.path.splitext(os.path.basename(take))[0])
            result = check_take(take, pitch, read_notes(stem + ".notes.csv"))
            result["open"], result["in_tab"] = open_in_tab(stem + ".events.csv", pitch, strings)
            report[f"{folder}/{os.path.basename(take)}"] = result
            for key in total:
                total[key] += result[key]
        summary = {"found": total["found"],
                   "agree": round(total["agree"] / max(total["found"], 1), 2),
                   "covered": round(total["covered_frames"] / max(total["pyin_frames"], 1), 2),
                   "open_in_tab": round(total["open"] / max(total["in_tab"], 1), 2)}
        summary["q08"] = summary["found"] > 0 and summary["covered"] >= MIN_COVERED and \
            summary["open_in_tab"] >= MIN_OPEN
        passed = passed and summary["q08"]
        report[folder] = summary
        print(folder, profile, summary, flush=True)
    with open(os.path.join(out, "low-strings.json"), "w") as file:
        json.dump(report, file, indent=1)
    return 0 if passed else 1


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    sys.exit(main(*sys.argv[1:]))
