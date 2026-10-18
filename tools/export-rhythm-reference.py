#!/usr/bin/env python3
"""Exports reference values of the PoC rhythm pipeline for the C++ port (E-12, AP5).

The PoC (rhythm_transcribe.py) tracks beats with madmom if it is installed; Scratch Notes uses its librosa
path instead (madmom's models are CC BY-NC-SA). This script holds that path of the PoC unchanged, cuts an
excerpt from a recording and writes the values of every stage:

  <name>-audio-22050.f32  the first 3 s at 22.05 kHz and <name>-audio-oenv{256,441}.f32 its onset strength
                          (only with --audio, for the onset strength test)
  <name>-oenv441.f32      onset strength, hop 441 (beat tracking)
  <name>-oenv256.f32      onset strength, hop 256 (meter features, clean-up)
  <name>-notes.csv        Basic Pitch notes after the level gate: start,end,pitch,amplitude
  <name>-tempo.csv        tempo candidates: bpm,share
  <name>-beats.csv        all beats, then the beats around the notes the pipeline uses
  <name>-features.csv     per beat: chord_change,onset,durational
  <name>-meter.csv        meter ranking: beats_per_bar,confidence,phase; phrase period
  <name>-events.csv       quantized events: position,duration (fractions of beats),weight,pitches,graces
  <name>-summary.txt      style, chord share, grid use

Usage: export-rhythm-reference.py RECORDING START_SECONDS LENGTH_SECONDS OUT_DIR NAME [--audio]
Needs: pip install librosa soundfile scipy onnxruntime; pip install --no-deps basic-pitch
"""
import os
import sys
import tempfile
from fractions import Fraction

import librosa
import numpy as np
import soundfile as sf

# ------------------------------------------------------------------ PoC (rhythm_transcribe.py), librosa path


def transcribe_notes(wav_path, y, sr, min_amp=0.3):
    from basic_pitch import ICASSP_2022_MODEL_PATH
    from basic_pitch.inference import Model, predict
    onnx = os.path.join(os.path.dirname(str(ICASSP_2022_MODEL_PATH)), "nmp.onnx")
    _, _, notes = predict(wav_path, Model(onnx), onset_threshold=0.5, frame_threshold=0.3,
                          minimum_note_length=80, melodia_trick=True)
    notes = np.array([(s, e, p, a) for s, e, p, a, _ in notes], dtype=float)
    hop = 512
    rms = librosa.feature.rms(y=y, hop_length=hop)[0]
    db = 20 * np.log10(rms + 1e-9)
    idx = np.clip((notes[:, 0] * sr / hop + 3).astype(int), 0, len(db) - 1)
    keep = (db[idx] > db.max() - 45) & (notes[:, 3] >= min_amp)
    notes = notes[keep]
    return notes[np.lexsort((notes[:, 2], notes[:, 0]))]


def tempo_candidates(act, fps, bpm_min=40, bpm_max=220, prior_center=100.0):
    a = act - act.mean()
    ac = np.correlate(a, a, mode="full")[len(a) - 1:]
    ac /= ac[0] + 1e-9
    lags = np.arange(1, len(ac))
    bpm = 60.0 * fps / lags
    ok = (bpm >= bpm_min) & (bpm <= bpm_max)
    prior = np.exp(-0.5 * (np.log2(bpm / prior_center) / 1.0) ** 2)
    score = np.where(ok, np.maximum(ac[1:], 0) * prior, 0)
    peaks = [i for i in range(1, len(score) - 1)
             if score[i] > 0 and score[i] >= score[i - 1] and score[i] >= score[i + 1]]
    peaks = sorted(peaks, key=lambda i: -score[i])
    cands, used = [], []
    for i in peaks:
        b = bpm[i]
        if any(abs(b - u) / u < 0.04 for u in used):
            continue
        used.append(b)
        cands.append((float(b), float(score[i])))
        if len(cands) == 4:
            break
    tot = sum(s for _, s in cands) or 1
    return [(b, s / tot) for b, s in cands]


def track_beats(y22):
    hop = 441
    oenv = librosa.onset.onset_strength(y=y22, sr=22050, hop_length=hop)
    cands = tempo_candidates(oenv, 22050 / hop)
    _, beats = librosa.beat.beat_track(onset_envelope=oenv, sr=22050, hop_length=hop, bpm=cands[0][0],
                                       tightness=40, units="time")
    return beats, oenv, cands


METER_PRIOR = {3: -0.4, 4: 0.0, 5: -1.2, 6: -0.6, 7: -1.5}


def beat_features(b, notes, oenv):
    nb = len(b)
    bb = np.append(b, b[-1] + (b[-1] - b[-2]))
    ot = librosa.times_like(oenv, sr=22050, hop_length=256)
    chroma = np.zeros((nb, 12))
    dur = np.zeros(nb)
    ons = np.zeros(nb)
    for i in range(nb):
        t0, t1 = bb[i] - 0.08, bb[i + 1] - 0.08
        for s, e, p, a in notes[(notes[:, 0] < t1) & (notes[:, 1] > t0)]:
            chroma[i, int(p) % 12] += a * (min(e, t1) - max(s, t0)) * (1.5 if p < 52 else 1.0)
        if np.any(np.abs(notes[:, 0] - bb[i]) < 0.1):
            low = notes[:, 2] <= np.percentile(notes[:, 2], 40)
            later = notes[(notes[:, 0] > bb[i] + 0.15) & low]
            dur[i] = (later[:, 0].min() - bb[i]) if len(later) else 1.0
        w = (ot > bb[i] - 0.07) & (ot < bb[i] + 0.07)
        ons[i] = oenv[w].max() if w.any() else 0
    cn = chroma / (np.linalg.norm(chroma, axis=1, keepdims=True) + 1e-9)
    change = np.r_[1.0, 1 - np.sum(cn[1:] * cn[:-1], axis=1)]
    return {"chord_change": change, "onset": ons, "durational": dur}, cn


def phrase_period(cn, lo=4, hi=32):
    S = cn @ cn.T
    hi = min(hi, len(cn) // 2)
    sims = {lag: float(np.mean(np.diag(S, lag))) for lag in range(1, hi + 1)}
    best = max(range(lo, hi + 1), key=lambda l: sims[l]) if hi >= lo else None
    if best is not None:
        best = min(l for l in range(lo, hi + 1) if sims[l] >= 0.93 * sims[best])
    strong = best is not None and sims[best] > 0.55 and sims[best] > np.mean(list(sims.values())) + 0.2
    return best, (sims[best] if best else 0), strong


def estimate_meter(feats, cn):
    z = lambda v: (v - v.mean()) / (v.std() + 1e-9)
    w = {"chord_change": 0.35, "onset": 0.2, "durational": 0.3, "downbeat_net": 0.15}
    tot = sum(w[k] for k in feats)
    comb = sum(w[k] / tot * z(v) for k, v in feats.items())
    P, psim, pstrong = phrase_period(cn)
    res = {}
    for M in range(2, 8):
        best = (-1e9, 0)
        for ph in range(M):
            idx = (np.arange(len(comb)) - ph) % M == 0
            a, o = comb[idx], comb[~idx]
            if len(a) < 2:
                continue
            t = (a.mean() - o.mean()) / np.sqrt(a.var() / len(a) + o.var() / len(o) + 1e-9)
            best = max(best, (t, ph))
        res[M] = best
    scores = {}
    for M in range(3, 8):
        s = res[M][0] + METER_PRIOR[M]
        if pstrong:
            s += 0.5 if P % M == 0 else -0.5
        scores[M] = s
    ms = np.array(list(scores.values()))
    p = np.exp(ms - ms.max())
    p /= p.sum()
    ranking = sorted([(M, float(pi), res[M][1]) for M, pi in zip(scores, p)], key=lambda x: -x[1])
    return ranking, (P, psim, pstrong)


GRID_PENALTY = {1: 0.0, 2: 0.5, 4: 1.5, 3: 2.0}
SIGMA = 0.07
STYLES = {
    "riff": dict(rel_amp=0.5, drop_w=0.06, drop_w_soft=0.12, late_low="merge", grace_beats=0.35, grace_ratio=0.7),
    "melody": dict(rel_amp=0.5, drop_w=0.06, drop_w_soft=0.12, late_low="drop", grace_beats=0.15, grace_ratio=0.5),
}


def _finish_cluster(c, rel_amp=0.5):
    arr = np.array(c["notes"])
    c["t"] = float(np.median(arr[:, 0]))
    c["end"] = float(np.percentile(arr[:, 1], 75))
    amax = arr[:, 3].max()
    c["pitches"] = sorted({int(p) for p in arr[arr[:, 3] >= rel_amp * amax, 2]})
    c["weight"] = float((arr[:, 3] * (arr[:, 1] - arr[:, 0])).sum())
    return c


def cluster_onsets(notes, tol=0.07, rel_amp=0.5):
    clusters = []
    for n in notes:
        if clusters and n[0] - clusters[-1]["t0"] < tol:
            clusters[-1]["notes"].append(n)
        else:
            clusters.append({"t0": n[0], "notes": [n]})
    return [_finish_cluster(c, rel_amp) for c in clusters]


def guess_style(notes):
    cl = cluster_onsets(notes)
    share = np.mean([len(c["pitches"]) >= 2 for c in cl]) if cl else 0
    return ("riff" if share >= 0.5 else "melody"), float(share)


def clean_events(clusters, oenv, b, st):
    ot = librosa.times_like(oenv, sr=22050, hop_length=256)
    pk = librosa.util.peak_pick(oenv, pre_max=3, post_max=3, pre_avg=10, post_avg=10, delta=0.3, wait=4)
    ref = np.percentile(oenv[pk], 75) if len(pk) else oenv.max()
    for c in clusters:
        w = (ot > c["t"] - 0.04) & (ot < c["t"] + 0.04)
        c["flux"] = float(oenv[w].max() / ref) if w.any() else 0.0
    kept = []
    for c in clusters:
        if c["weight"] < st["drop_w"] or (c["weight"] < st["drop_w_soft"] and c["flux"] < 0.6):
            continue
        if kept and c["flux"] < 0.15:
            prev = kept[-1]
            ringing = {p % 12 for p in prev["pitches"]} if prev["end"] > c["t"] - 0.05 else set()
            if {p % 12 for p in c["pitches"]} <= ringing:
                continue
            if c["t"] - prev["t"] < 0.2:
                if st["late_low"] == "merge":
                    prev["notes"] += c["notes"]
                    _finish_cluster(prev, st["rel_amp"])
                    prev["t"] = prev["t0"]
                    continue
                if max(c["pitches"]) <= min(prev["pitches"]) - 5:
                    continue
        kept.append(c)
    ibi = np.median(np.diff(b))
    events, main = [], None
    for c in reversed(kept):
        c.setdefault("graces", [])
        if (main is not None and main["t"] - c["t"] < st["grace_beats"] * ibi
                and c["weight"] < st["grace_ratio"] * main["weight"]):
            main["graces"].insert(0, c["pitches"][:2])
            continue
        events.append(c)
        main = c
    return events[::-1]


def beat_pos(t, b):
    t = np.asarray(t, dtype=float)
    p = np.interp(t, b, np.arange(len(b)))
    lo, hi = t < b[0], t > b[-1]
    p[lo] = (t[lo] - b[0]) / (b[1] - b[0])
    p[hi] = len(b) - 1 + (t[hi] - b[-1]) / (b[-1] - b[-2])
    return p


def quantize(clusters, b):
    pos = beat_pos([c["t"] for c in clusters], b)
    for c, p in zip(clusters, pos):
        c["pos"] = p
    by_beat = {}
    for c in clusters:
        by_beat.setdefault(int(np.floor(c["pos"] + 0.125)), []).append(c)
    grid_use = {g: 0 for g in GRID_PENALTY}
    for k, cs in by_beat.items():
        best = None
        for g, pen in GRID_PENALTY.items():
            fr = np.array([c["pos"] - k for c in cs])
            q = np.round(fr * g) / g
            if g == 3 and len(cs) == 1:
                pen += 2.0
            cost = np.sum((fr - q) ** 2) / SIGMA ** 2 + pen + 3.0 * (len(q) - len(set(q)))
            if best is None or cost < best[0]:
                best = (cost, g, q)
        _, g, q = best
        grid_use[g] += 1
        for c, qq in zip(cs, q):
            c["qpos"] = Fraction(k) + Fraction(int(round(qq * g)), g)
    out = {}
    for c in clusters:
        if c["qpos"] not in out or c["weight"] > out[c["qpos"]]["weight"]:
            out[c["qpos"]] = c
    events = [out[k] for k in sorted(out)]
    qend = beat_pos([e["end"] for e in events], b)
    for i, e in enumerate(events):
        nxt = events[i + 1]["qpos"] if i + 1 < len(events) else None
        g = 3 if e["qpos"].denominator == 3 else 2
        end_q = max(Fraction(int(round(qend[i] * g)), g), e["qpos"] + Fraction(1, g))
        if nxt is None:
            dur = max(end_q - e["qpos"], Fraction(1, 2))
        else:
            gap = nxt - end_q
            dur = (nxt - e["qpos"]) if gap <= Fraction(1, 2) else (end_q - e["qpos"])
        e["qdur"] = dur
    return events, grid_use


# ------------------------------------------------------------------ export


def main():
    recording, start, length, out_dir, name = sys.argv[1:6]
    with_audio = "--audio" in sys.argv[6:]
    y, sr = sf.read(recording, always_2d=True, start=0)
    y = y.mean(axis=1).astype(np.float32)
    first, count = int(float(start) * sr), int(float(length) * sr)
    y = y[first:first + count]
    if sr != 44100:
        y = librosa.resample(y, orig_sr=sr, target_sr=44100)
        sr = 44100
    y22 = librosa.resample(y, orig_sr=sr, target_sr=22050)
    os.makedirs(out_dir, exist_ok=True)
    base = os.path.join(out_dir, name)
    if with_audio:
        short = y22[:3 * 22050]
        short.astype("<f4").tofile(base + "-audio-22050.f32")
        for hop in (256, 441):
            librosa.onset.onset_strength(y=short, sr=22050, hop_length=hop).astype("<f4").tofile(
                f"{base}-audio-oenv{hop}.f32")

    with tempfile.TemporaryDirectory() as tmp:
        mono = os.path.join(tmp, "mono.wav")
        sf.write(mono, y, sr)
        notes = transcribe_notes(mono, y, sr)

    beats_all, oenv441, cands = track_beats(y22)
    ibi0 = np.median(np.diff(beats_all))
    b = beats_all[(beats_all >= notes[0, 0] - 0.6 * ibi0) & (beats_all <= notes[:, 0].max() + ibi0)]
    oenv256 = librosa.onset.onset_strength(y=y22, sr=22050, hop_length=256)
    feats, cn = beat_features(b, notes, oenv256)
    ranking, (period, similarity, strong) = estimate_meter(feats, cn)
    style, share = guess_style(notes)
    st = STYLES[style]
    clusters = clean_events(cluster_onsets(notes, rel_amp=st["rel_amp"]), oenv256, b, st)
    events, grid_use = quantize(clusters, b)

    oenv441.astype("<f4").tofile(base + "-oenv441.f32")
    oenv256.astype("<f4").tofile(base + "-oenv256.f32")
    np.savetxt(base + "-notes.csv", notes, delimiter=",", fmt="%.9g", header="start,end,pitch,amplitude",
               comments="")
    np.savetxt(base + "-tempo.csv", np.array(cands), delimiter=",", fmt="%.9g", header="bpm,share", comments="")
    with open(base + "-beats.csv", "w") as f:
        f.write("all\n" + ",".join(f"{t:.9g}" for t in beats_all) + "\n")
        f.write("used\n" + ",".join(f"{t:.9g}" for t in b) + "\n")
    np.savetxt(base + "-features.csv",
               np.stack([feats["chord_change"], feats["onset"], feats["durational"]], axis=1),
               delimiter=",", fmt="%.9g", header="chord_change,onset,durational", comments="")
    with open(base + "-meter.csv", "w") as f:
        f.write("beats_per_bar,confidence,phase\n")
        for M, p, ph in ranking:
            f.write(f"{M},{p:.9g},{ph}\n")
        f.write(f"phrase,{period},{similarity:.9g},{int(strong)}\n")
    with open(base + "-events.csv", "w") as f:
        f.write("position,duration,weight,pitches,graces\n")
        for e in events:
            graces = "|".join(" ".join(str(p) for p in g) for g in e["graces"])
            f.write(f"{e['qpos']},{e['qdur']},{e['weight']:.9g},{' '.join(str(p) for p in e['pitches'])},{graces}\n")
    with open(base + "-summary.txt", "w") as f:
        f.write(f"style {style}\nchord_share {share:.9g}\n")
        f.write("grid " + " ".join(f"{g}:{n}" for g, n in grid_use.items()) + "\n")
    print(f"{len(notes)} notes, {len(beats_all)} beats ({len(b)} used), tempo {cands[0][0]:.1f}, "
          f"meter {ranking[0][0]} ({ranking[0][1]:.2f}), {style}, {len(events)} events")


if __name__ == "__main__":
    main()
