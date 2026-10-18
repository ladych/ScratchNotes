#pragma once

#include "model/InstrumentProfile.h"
#include "transcription/LevelGate.h"
#include "transcription/Note.h"
#include "transcription/NoteCreation.h"
#include "transcription/PitchAnalysis.h"
#include "transcription/PitchModel.h"

#include <optional>
#include <span>
#include <vector>

namespace snotes::transcription
{

struct TranscriptionSettings
{
    /// Analyse an octave higher and move the notes back down (F-TX-03).
    bool octaveUp = false;
    /// Notes outside the range of the instrument are dropped (F-TX-08).
    int lowestPitch = 0;
    int highestPitch = 127;
    NoteSettings notes;
    GateSettings gate;
};

/// Settings for an instrument: its range from the lowest open string to the highest fret of the highest
/// string, and the octave shift of the bass profiles.
[[nodiscard]] TranscriptionSettings settingsFor(const model::InstrumentProfile& profile);

/// Detects the notes of a DI recording (F-TX-02): resampling to the rate of the model, Basic Pitch, note
/// creation, level gate, octave shift and range of the instrument. The notes are sorted by start and pitch.
/// Returns nothing if the progress callback cancelled (F-TX-01).
[[nodiscard]] std::optional<std::vector<Note>> transcribe(std::span<const float> audio,
                                                          double sampleRate,
                                                          IPitchModel& model,
                                                          const TranscriptionSettings& settings,
                                                          const ProgressCallback& progress);

} // namespace snotes::transcription
