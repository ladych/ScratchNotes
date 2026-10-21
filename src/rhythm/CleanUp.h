#pragma once

#include "transcription/Note.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace snotes::rhythm
{

/// Notes that start together: one strike of a chord or a single note.
struct Cluster
{
    /// Start of the first note.
    double firstSeconds = 0.0;
    /// Median start of the notes.
    double seconds = 0.0;
    /// 75th percentile of the ends.
    double endSeconds = 0.0;
    /// The pitches at least half as strong as the strongest one, ascending.
    std::vector<int> pitches;
    /// Sum of amplitude times duration.
    double weight = 0.0;
    /// Attack strength in the audio relative to the typical onset (clean-up only).
    double flux = 0.0;
    std::vector<transcription::Note> notes;
    /// Grace notes before the cluster, each with up to two pitches.
    std::vector<std::vector<int>> graces;
};

/// Groups the notes (sorted by start) whose start is within 70 ms of the first note of a group.
[[nodiscard]] std::vector<Cluster> clusterOnsets(const std::vector<transcription::Note>& notes,
                                                 double relativeAmplitude);

enum class Style : std::uint8_t
{
    /// Chords and power chords: late notes melt into the chord.
    riff,
    /// Single notes and legato: late low resonances are dropped, grace notes are rarer.
    melody,
};

struct StyleGuess
{
    Style style = Style::melody;
    /// Share of the clusters with two or more pitches.
    double chordShare = 0.0;

    bool operator==(const StyleGuess&) const = default;
};

/// riff if at least half of the strikes are chords (F-TX-07).
[[nodiscard]] StyleGuess guessStyle(const std::vector<transcription::Note>& notes);

/// The clean-up rules of the PoC (STYLES in rhythm_transcribe.py).
struct CleanUpRules
{
    double relativeAmplitude = 0.5;
    /// Clusters lighter than this are dropped, those up to dropSoftWeight if their attack is weak.
    double dropWeight = 0.06;
    double dropSoftWeight = 0.12;
    /// riff: a note without attack shortly after a cluster joins it; melody: it is dropped if it is low.
    bool mergeLateNotes = true;
    /// A light cluster this many beats before a heavier one is its grace note.
    double graceBeats = 0.35;
    double graceWeightRatio = 0.7;
    /// Not in the PoC: a cluster without attack whose notes all lie below a ringing note or in its pitch
    /// classes is a ghost of the model (subharmonics on the 7-string, notes split at its window seams).
    bool dropGhostsUnderRinging = true;
};

[[nodiscard]] CleanUpRules rulesFor(Style style);

/// Drops ghost notes, ringing overtones and slide starts and turns light notes just before strong ones into
/// grace notes (F-TX-07, PoC clean_events). envelope is the onset strength with kDetailHop, beats the beat
/// times around the notes (at least two).
[[nodiscard]] std::vector<Cluster> cleanUp(std::vector<Cluster> clusters,
                                           std::span<const float> envelope,
                                           std::span<const double> beats,
                                           const CleanUpRules& rules);

/// librosa.util.peak_pick (greedy): frames that are the maximum of [n - preMax, n + postMax) and at least
/// delta above the mean of [n - preAverage, n + postAverage), at least wait + 1 frames apart.
struct PeakPicking
{
    int preMax = 3;
    int postMax = 3;
    int preAverage = 10;
    int postAverage = 10;
    double delta = 0.3;
    int wait = 4;
};

[[nodiscard]] std::vector<std::size_t> pickPeaks(std::span<const float> envelope,
                                                 const PeakPicking& settings);

} // namespace snotes::rhythm
