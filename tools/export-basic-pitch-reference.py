#!/usr/bin/env python3
"""Exports reference values of Basic Pitch 0.4.0 for the C++ port (E-12).

Cuts an excerpt from a recording, resamples it to 22.05 kHz with librosa (as Basic Pitch does) and writes
  <name>-22050.f32       the excerpt the model sees (float32, little endian)
  <name>-frames.f32      unwrapped note activations, frames x 88 (float32)
  <name>-onsets.f32      unwrapped onset activations, frames x 88 (float32)
  <name>-notes.csv       note events with the parameters of the PoC

Usage: export-basic-pitch-reference.py RECORDING START_SECONDS LENGTH_SECONDS OUT_DIR NAME
Needs: pip install basic-pitch onnxruntime librosa soundfile (TensorFlow is not needed).
"""
import os
import sys
import tempfile

import librosa
import numpy as np
import soundfile as sf
from basic_pitch import ICASSP_2022_MODEL_PATH, note_creation
from basic_pitch.constants import AUDIO_SAMPLE_RATE, FFT_HOP
from basic_pitch.inference import Model, run_inference

# Parameters of the PoC (rhythm_transcribe.py).
ONSET_THRESHOLD = 0.5
FRAME_THRESHOLD = 0.3
MINIMUM_NOTE_MS = 80


def main():
    recording, start, length, out_dir, name = sys.argv[1:6]
    audio, _ = librosa.load(recording, sr=AUDIO_SAMPLE_RATE, mono=True, offset=float(start), duration=float(length))
    audio = audio.astype(np.float32)
    os.makedirs(out_dir, exist_ok=True)
    base = os.path.join(out_dir, name)
    audio.astype("<f4").tofile(base + "-22050.f32")

    with tempfile.TemporaryDirectory() as tmp:
        excerpt = os.path.join(tmp, "excerpt.wav")
        sf.write(excerpt, audio, AUDIO_SAMPLE_RATE, subtype="FLOAT")
        model = Model(os.path.join(os.path.dirname(str(ICASSP_2022_MODEL_PATH)), "nmp.onnx"))
        output = run_inference(excerpt, model)

    output["note"].astype("<f4").tofile(base + "-frames.f32")
    output["onset"].astype("<f4").tofile(base + "-onsets.f32")

    min_note_len = int(np.round(MINIMUM_NOTE_MS / 1000 * (AUDIO_SAMPLE_RATE / FFT_HOP)))
    events = note_creation.output_to_notes_polyphonic(
        output["note"].copy(), output["onset"].copy(), onset_thresh=ONSET_THRESHOLD, frame_thresh=FRAME_THRESHOLD,
        min_note_len=min_note_len, infer_onsets=True, max_freq=None, min_freq=None, melodia_trick=True)
    times = note_creation.model_frames_to_time(output["note"].shape[0])
    with open(base + "-notes.csv", "w") as f:
        f.write("start_frame,end_frame,pitch,amplitude,start_seconds,end_seconds\n")
        for start_frame, end_frame, pitch, amplitude in events:
            f.write(f"{start_frame},{end_frame},{pitch},{amplitude:.7f},"
                    f"{times[start_frame]:.7f},{times[end_frame]:.7f}\n")
    print(f"{len(audio)} samples, {output['note'].shape[0]} frames, {len(events)} notes")


if __name__ == "__main__":
    main()
