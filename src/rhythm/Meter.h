#pragma once

#include "transcription/Note.h"

#include <array>
#include <span>
#include <vector>

namespace snotes::rhythm
{

/// What marks a beat as the first of a bar, one value per beat (PoC beat_features).
struct BeatFeatures
{
    /// 1 - cosine similarity of the pitch classes sounding from one beat to the next.
    std::vector<double> chordChange;
    /// Strongest onset within 70 ms of the beat.
    std::vector<double> onset;
    /// Seconds to the next low note, for beats with a note on them: long notes start bars.
    std::vector<double> durational;
    /// Normalised pitch classes per beat, bass notes weighted 1.5 times.
    std::vector<std::array<double, 12>> chroma;
};

/// beats are the beat times around the notes, envelope the onset strength with kDetailHop.
[[nodiscard]] BeatFeatures beatFeatures(std::span<const double> beats,
                                        const std::vector<transcription::Note>& notes,
                                        std::span<const float> envelope);

/// The number of beats after which the harmony repeats (PoC phrase_period).
struct Phrase
{
    /// 0 if the recording is shorter than eight beats.
    int beats = 0;
    double similarity = 0.0;
    /// Clearly repeating: then bar lengths that do not divide the phrase are unlikely.
    bool strong = false;

    bool operator==(const Phrase&) const = default;
};

[[nodiscard]] Phrase phrasePeriod(const std::vector<std::array<double, 12>>& chroma);

struct MeterCandidate
{
    /// 3 to 7 (quarter notes per bar).
    int beatsPerBar = 4;
    /// 0..1, the candidates add up to 1.
    double confidence = 0.0;
    /// Index of the first beat that starts a bar, 0 .. beatsPerBar - 1.
    int phase = 0;

    bool operator==(const MeterCandidate&) const = default;
};

struct MeterEstimate
{
    /// All meters from 3/4 to 7/4, most likely first (F-TX-06).
    std::vector<MeterCandidate> ranking;
    Phrase phrase;

    bool operator==(const MeterEstimate&) const = default;
};

/// Meter suggestions (F-TX-06, PoC estimate_meter): for every meter and phase, how much the features on the
/// supposed first beats of the bars stand out (t statistic), plus a prior for common meters and the
/// plausibility check against the phrase length, as probabilities.
[[nodiscard]] MeterEstimate estimateMeter(const BeatFeatures& features);

} // namespace snotes::rhythm
