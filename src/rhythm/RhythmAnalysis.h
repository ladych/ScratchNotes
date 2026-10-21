#pragma once

#include "rhythm/BeatTracker.h"
#include "rhythm/CleanUp.h"
#include "rhythm/Meter.h"
#include "rhythm/Quantizer.h"
#include "transcription/Note.h"

#include <optional>
#include <span>
#include <vector>

namespace snotes::rhythm
{

/// Onset strength of a recording at the hops of the beat tracker and of the details.
struct Onsets
{
    std::vector<float> beat;
    std::vector<float> detail;

    bool operator==(const Onsets&) const = default;
};

/// audio at kAnalysisRate.
[[nodiscard]] Onsets measureOnsets(std::span<const float> audio);

/// One recorded voice: its notes and onsets in the time of the recording, which starts at startSeconds of the
/// song.
struct RhythmTrack
{
    std::vector<transcription::Note> notes;
    Onsets onsets;
    double startSeconds = 0.0;
};

/// The beat grid all voices share (F-TX-05).
struct BeatGrid
{
    /// The tempo candidates; the first one fixed the metric level (F-TX-04).
    std::vector<TempoCandidate> tempoCandidates;
    /// Beat times in song seconds, from shortly before the first note to shortly after the last one.
    std::vector<double> beats;
    MeterEstimate meter;

    bool operator==(const BeatGrid&) const = default;
};

/// Tracks the beats on the onsets of all tracks together and suggests the meter from the notes of all of
/// them (F-TX-04 to 06). Nothing if there are no notes or fewer than two beats around them.
[[nodiscard]] std::optional<BeatGrid> commonBeatGrid(const std::vector<RhythmTrack>& tracks,
                                                     const BeatTrackerSettings& settings);

/// Tempo in BPM from each beat to the next (one value fewer than beats).
[[nodiscard]] std::vector<double> tempoCurve(const std::vector<double>& beats);

/// The rhythm of one voice on the common grid.
struct VoiceRhythm
{
    StyleGuess style;
    Quantization quantization;

    bool operator==(const VoiceRhythm&) const = default;
};

/// Cleans up the notes of a track with the rules of its style and quantizes them on the grid (F-TX-07,
/// F-TX-09). Event times are in the time of the recording, positions count from the first beat of the grid.
[[nodiscard]] VoiceRhythm voiceRhythm(const RhythmTrack& track, const BeatGrid& grid);

} // namespace snotes::rhythm
