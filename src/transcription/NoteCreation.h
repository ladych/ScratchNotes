#pragma once

#include "transcription/BasicPitch.h"

#include <vector>

namespace snotes::transcription
{

/// Parameters of the note creation. The defaults are those of the PoC (rhythm_transcribe.py).
struct NoteSettings
{
    double onsetThreshold = 0.5;
    double frameThreshold = 0.3;
    /// Notes must be longer than this many frames (80 ms in the PoC).
    int minimumFrames = 7;
    bool inferOnsets = true;
    bool melodiaTrick = true;
    /// Frames below the frame threshold a note may contain before it ends.
    int energyTolerance = 11;
};

/// A note in model frames, as output_to_notes_polyphonic returns it.
struct FrameNote
{
    int startFrame = 0;
    /// Exclusive.
    int endFrame = 0;
    /// MIDI note.
    int pitch = 0;
    /// Mean note activation over the note, 0..1.
    float amplitude = 0.0F;

    bool operator==(const FrameNote&) const = default;
};

/// Port of basic_pitch.note_creation.output_to_notes_polyphonic (0.4.0) without frequency limits: notes
/// start at peaks of the onset activations, last while the note activation stays above the frame threshold,
/// and the "melodia trick" adds notes for the energy that is left (F-TX-02, E-12). Same order as Python:
/// onset notes from the last onset backwards, then melodia notes.
[[nodiscard]] std::vector<FrameNote> createNotes(const Activations& activations,
                                                 const NoteSettings& settings);

/// Model frames for a minimum note length in milliseconds, rounded as predict() in Basic Pitch does.
[[nodiscard]] int minimumFramesFor(double milliseconds);

/// Port of model_frames_to_time: the time of a frame in seconds, including Basic Pitch's correction for the
/// windows.
[[nodiscard]] double frameToSeconds(int frame);

} // namespace snotes::transcription
