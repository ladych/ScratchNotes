#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace snotes::rhythm
{

/// A tempo that fits the onsets, with its share of the evidence of all candidates (0..1).
struct TempoCandidate
{
    double bpm = 0.0;
    double share = 0.0;

    bool operator==(const TempoCandidate&) const = default;
};

/// Up to four tempos from the autocorrelation of an onset envelope, best first (PoC tempo_candidates):
/// 40 to 220 BPM, weighted towards 100 BPM by a log-normal prior one octave wide, candidates closer than
/// 4 % to a better one skipped. The first one fixes the metric level for the whole recording (F-TX-04).
[[nodiscard]] std::vector<TempoCandidate> tempoCandidates(std::span<const float> envelope,
                                                          double framesPerSecond);

struct BeatTrackerSettings
{
    /// How strongly the tracker keeps to the tempo (librosa's tightness).
    double tightness = 40.0;
    /// The span of the beat before, relative to the tempo: librosa allows 1/2 to 2 beats; Scratch Notes
    /// lets the tempo drift by 40 % (F-TX-04).
    double shortestBeat = 1.0 / 1.4;
    double longestBeat = 1.0 / 0.6;
};

/// Beats of librosa's search window, for the comparison with the PoC (E-12).
inline constexpr BeatTrackerSettings kLibrosaWindow{
    .tightness = 40.0, .shortestBeat = 0.5, .longestBeat = 2.0};

/// Beat times in seconds by dynamic programming (Ellis 2007), as librosa.beat.beat_track with a fixed tempo
/// (F-TX-04): the envelope smoothed with a Gaussian window of one beat, the best chain of beats whose
/// spacing stays near the tempo, weak beats at the start and the end removed.
[[nodiscard]] std::vector<double> trackBeats(std::span<const float> envelope,
                                             double framesPerSecond,
                                             double bpm,
                                             const BeatTrackerSettings& settings);

/// Steps of trackBeats, public for their tests.
/// librosa's __last_beat: the last local maximum of the cumulative score at least half their median; the
/// last frame if there is none. cumulative needs two values.
[[nodiscard]] std::ptrdiff_t lastBeat(const std::vector<double>& cumulative);
/// librosa's __trim_beats: clears the frames at both ends whose local score stays at or below half the RMS
/// of the smoothed local scores of the beats.
void trimBeats(const std::vector<double>& score, std::vector<bool>& beats);

} // namespace snotes::rhythm
