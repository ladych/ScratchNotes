#!/usr/bin/env python3
"""Measures the transcription quality of scratchnotes-transcribe on the PoC recordings (Q-06, Q-07, E-10).

Runs the command line tool on RECORDINGS/test.wav (riff), Clean.wav (improvisation, DI) and distortion.wav
(the same improvisation through the amp) and computes the figures of the PoC (rhythm_transcribe.py
self_check and vergleich_clean_vs_distortion.py):

  onset F1     onsets librosa finds in the audio vs. the quantized events on the beat grid, +-70 ms
  pitch class  chroma of the recording vs. the events played back with a plucked-string synth
  beats F      beats of the DI vs. the processed recording after removing the latency, +-70 ms
  speed        seconds of transcription per second of audio (Q-06: at most 1)

Writes OUT/metrics.json and fails if a figure misses Q-07.
Usage: evaluate-transcription.py CLI RECORDINGS OUT
Needs: pip install librosa soundfile scipy mir_eval
"""
import json
import os
import re
import subprocess
import sys
from fractions import Fraction

import librosa
import mir_eval
import numpy as np
import scipy.signal as ss
import soundfile as sf

# Q-07: at least the values of the PoC.
TARGETS = {"riff": {"onset_f1": 0.72, "pitch_class": 0.86},
           "improvisation": {"onset_f1": 0.84, "pitch_class": 0.82},
           "beats_f": 0.90}


def load_audio(path):
    y, sr = sf.read(path, always_2d=True)
    y = y.mean(axis=1).astype(np.float32)
    if sr != 44100:
        y = librosa.resample(y, orig_sr=sr, target_sr=44100)
    return y, 44100


def read_beats(path):
    rows = np.loadtxt(path, delimiter=",", skiprows=1, usecols=(1,), ndmin=1)
    return np.asarray(rows, dtype=float)


def read_events(path):
    events = []
    with open(path) as f:
        next(f)
        for line in f:
            position, duration, _, weight, pitches, graces = line.rstrip("\n").split(",")
            events.append({"qpos": Fraction(position), "qdur": Fraction(duration), "weight": float(weight),
                           "pitches": [int(p) for p in pitches.split()],
                           "graces": [[int(p) for p in g.split()] for g in graces.split("|")] if graces else []})
    return events


def beat_to_time(q, b):
    q = np.asarray(q, dtype=float)
    t = np.interp(q, np.arange(len(b)), b)
    lo, hi = q < 0, q > len(b) - 1
    t[lo] = b[0] + q[lo] * (b[1] - b[0])
    t[hi] = b[-1] + (q[hi] - (len(b) - 1)) * (b[-1] - b[-2])
    return t


def pluck(f, dur, sr, amp, rng):
    n_period = max(2, int(round(sr / f - 0.5)))
    n = int((dur + 0.25) * sr)
    x = np.zeros(n)
    x[:n_period] = np.convolve(rng.uniform(-1, 1, n_period), [0.5, 0.5], "same") * amp
    a = np.zeros(n_period + 2)
    a[0] = 1.0
    a[n_period] = a[n_period + 1] = -0.4985
    y = ss.lfilter([1.0], a, x)
    k, release = int(dur * sr), int(0.08 * sr)
    env = np.ones(n)
    env[k:k + release] = np.linspace(1, 0, len(env[k:k + release]))
    env[k + release:] = 0
    return y * env


def synthesize(events, b, sr, n_samples, seed=0):
    rng = np.random.default_rng(seed)
    out = np.zeros(n_samples)
    for e in events:
        t0, t1 = beat_to_time([float(e["qpos"]), float(e["qpos"] + e["qdur"])], b)
        amp = (0.25 + 0.5 * min(1.0, e["weight"] / 1.5)) / np.sqrt(len(e["pitches"]))
        notes = [(t0, t1 - t0, p, amp) for p in e["pitches"]]
        for j, gp in enumerate(e["graces"]):
            notes += [(t0 - 0.05 * (len(e["graces"]) - j), 0.05, p, 0.6 * amp) for p in gp]
        for ts, d, p, a in notes:
            s = int(ts * sr)
            if s < 0 or s >= n_samples:
                continue
            v = pluck(librosa.midi_to_hz(p), max(0.05, d), sr, a, rng)
            m = min(len(v), n_samples - s)
            out[s:s + m] += v[:m]
    return out


def self_check(y, sr, events, b):
    """PoC self_check: onsets in the audio vs. the transcription, pitch classes vs. its playback."""
    synth = synthesize(events, b, sr, len(y))
    y22 = librosa.resample(y, orig_sr=sr, target_sr=22050)
    s22 = librosa.resample(synth.astype(np.float32), orig_sr=sr, target_sr=22050)
    est = beat_to_time([float(e["qpos"]) for e in events], b)
    ref = librosa.onset.onset_detect(y=y22, sr=22050, units="time")
    f1, _, _ = mir_eval.onset.f_measure(ref, np.sort(est), window=0.07)
    cr = librosa.feature.chroma_cqt(y=y22, sr=22050, hop_length=2048)
    cs = librosa.feature.chroma_cqt(y=s22, sr=22050, hop_length=2048)
    n = min(cr.shape[1], cs.shape[1])
    cr, cs = cr[:, :n], cs[:, :n]
    rms = librosa.feature.rms(y=y22, hop_length=2048)[0][:n]
    active = 20 * np.log10(rms + 1e-9) > 20 * np.log10(rms.max()) - 35
    cos = np.sum(cr * cs, 0) / (np.linalg.norm(cr, axis=0) * np.linalg.norm(cs, axis=0) + 1e-9)
    return {"onset_f1": round(float(f1), 2), "pitch_class": round(float(np.mean(cos[active])), 2)}


def latency(clean, processed):
    """Delay of the processed recording, from the cross-correlation of the onset curves (+-1 s)."""
    oc = librosa.onset.onset_strength(y=librosa.resample(clean, orig_sr=44100, target_sr=22050), sr=22050,
                                      hop_length=128)
    od = librosa.onset.onset_strength(y=librosa.resample(processed, orig_sr=44100, target_sr=22050), sr=22050,
                                      hop_length=128)
    n = min(len(oc), len(od))
    oc, od = oc[:n], od[:n]
    x = ss.correlate(od - od.mean(), oc - oc.mean(), mode="full")
    lags = np.arange(-n + 1, n)
    near = np.abs(lags) < int(22050 / 128)
    return float(lags[near][np.argmax(x[near])] * 128 / 22050)


def main():
    cli, recordings, out = sys.argv[1:4]
    os.makedirs(out, exist_ok=True)
    names = {"riff": "test", "improvisation": "Clean", "processed": "distortion"}
    files = [os.path.join(recordings, f"{stem}.wav") for stem in names.values()]
    run = subprocess.run([cli, "--out", out, "--separate", *files], check=True, capture_output=True, text=True)
    print(run.stdout, end="")
    speed = {}
    for stem in names.values():
        match = re.search(rf"{stem}\.wav: \d+ notes, ([\d.]+) s audio in ([\d.]+) s", run.stdout)
        speed[stem] = round(float(match.group(2)) / float(match.group(1)), 3)

    metrics = {"speed_seconds_per_second": speed}
    audio = {}
    for key, stem in names.items():
        y, sr = load_audio(os.path.join(recordings, f"{stem}.wav"))
        audio[key] = y
        if key != "processed":
            b = read_beats(os.path.join(out, f"{stem}.beats.csv"))
            metrics[key] = self_check(y, sr, read_events(os.path.join(out, f"{stem}.events.csv")), b)
    delay = latency(audio["improvisation"], audio["processed"])
    clean_beats = read_beats(os.path.join(out, "Clean.beats.csv"))
    processed_beats = read_beats(os.path.join(out, "distortion.beats.csv")) - delay
    metrics["latency_ms"] = round(1000 * delay, 1)
    metrics["beats_f"] = round(float(mir_eval.beat.f_measure(mir_eval.beat.trim_beats(clean_beats),
                                                             mir_eval.beat.trim_beats(processed_beats))), 2)

    failures = [f"{key} {figure} {metrics[key][figure]} < {target}"
                for key in ("riff", "improvisation") for figure, target in TARGETS[key].items()
                if metrics[key][figure] < target]
    if metrics["beats_f"] < TARGETS["beats_f"]:
        failures.append(f"beats F {metrics['beats_f']} < {TARGETS['beats_f']}")
    failures += [f"{stem} speed {value} > 1" for stem, value in speed.items() if value > 1.0]
    metrics["q07_failures"] = failures
    with open(os.path.join(out, "metrics.json"), "w") as f:
        json.dump(metrics, f, indent=2)
    print(json.dumps(metrics, indent=2))
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
