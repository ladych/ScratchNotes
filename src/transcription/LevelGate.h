#pragma once

#include "transcription/Note.h"

#include <span>
#include <vector>

namespace snotes::transcription
{

/// Thresholds of the level gate; the defaults are those of the PoC.
struct GateSettings
{
    /// Notes must start less than this far below the loudest moment of the recording.
    double rangeDb = 45.0;
    float minimumAmplitude = 0.3F;
    /// Before the first note less than this far below the loudest moment, quieter notes are the noise of
    /// the lead-in (a hand on the strings, an open string ringing on its own) and dropped.
    double leadInRangeDb = 30.0;
};

/// Drops the notes the model hallucinates in (almost) digital silence and the very uncertain ones
/// (F-TX-02), and the quiet ones before the playing starts. Port of the gate in the PoC
/// (rhythm_transcribe.py): the RMS level shortly after the start of the note, measured in frames of 46 ms
/// every 11.6 ms. The audio is the one the model heard, at 22.05 kHz.
[[nodiscard]] std::vector<Note>
applyLevelGate(const std::vector<Note>& notes, std::span<const float> audio, const GateSettings& settings);

} // namespace snotes::transcription
