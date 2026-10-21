#include "rhythm/Meter.h"

#include "rhythm/OnsetStrength.h"
#include "rhythm/Statistics.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>

namespace snotes::rhythm
{
namespace
{

/// Notes are counted for a beat from shortly before it: attacks are early rather than late.
constexpr double kBeatLead = 0.08;
constexpr int kBassBelowPitch = 52;
constexpr double kBassWeight = 1.5;
/// A beat counts as having a note on it within this distance.
constexpr double kNoteOnBeat = 0.1;
constexpr double kBassPercentile = 40.0;
constexpr double kNextBassAfter = 0.15;
constexpr double kOnsetWindow = 0.07;
constexpr double kTiny = 1e-9;

using Chroma = std::array<double, 12>;

Chroma pitchClasses(const std::vector<transcription::Note>& notes, double from, double to)
{
    Chroma chroma{};
    for (const auto& note : notes)
        if (note.startSeconds < to && note.endSeconds > from)
            chroma.at(static_cast<std::size_t>(note.pitch % 12)) +=
                note.amplitude * (std::min(note.endSeconds, to) - std::max(note.startSeconds, from)) *
                (note.pitch < kBassBelowPitch ? kBassWeight : 1.0);
    const double norm =
        std::sqrt(std::inner_product(chroma.begin(), chroma.end(), chroma.begin(), 0.0)) + kTiny;
    for (auto& value : chroma)
        value /= norm;
    return chroma;
}

double durational(const std::vector<transcription::Note>& notes, double beat, double bassPitch)
{
    if (std::ranges::none_of(
            notes, [beat](const auto& note) { return std::abs(note.startSeconds - beat) < kNoteOnBeat; }))
        return 0.0;
    double next = 1.0;
    for (const auto& note : notes)
        if (note.startSeconds > beat + kNextBassAfter && note.pitch <= bassPitch)
        {
            next = note.startSeconds - beat;
            break;
        }
    return next;
}

double strongestOnset(std::span<const float> envelope, double beat)
{
    double strongest = 0.0;
    for (std::size_t frame = 0; frame < envelope.size(); ++frame)
    {
        const double seconds = frameSeconds(frame, kDetailHop);
        if (seconds > beat - kOnsetWindow && seconds < beat + kOnsetWindow)
            strongest = std::max(strongest, static_cast<double>(envelope[frame]));
    }
    return strongest;
}

double dot(const Chroma& a, const Chroma& b)
{
    return std::inner_product(a.begin(), a.end(), b.begin(), 0.0);
}

} // namespace

BeatFeatures beatFeatures(std::span<const double> beats,
                          const std::vector<transcription::Note>& notes,
                          std::span<const float> envelope)
{
    std::vector<double> bounds{beats.begin(), beats.end()};
    bounds.push_back(bounds.back() + (bounds.back() - bounds[bounds.size() - 2]));
    std::vector<double> pitches;
    pitches.reserve(notes.size());
    for (const auto& note : notes)
        pitches.push_back(note.pitch);
    const double bassPitch = percentile(pitches, kBassPercentile);
    BeatFeatures features;
    for (std::size_t beat = 0; beat < beats.size(); ++beat)
    {
        features.chroma.push_back(
            pitchClasses(notes, bounds[beat] - kBeatLead, bounds[beat + 1] - kBeatLead));
        features.durational.push_back(durational(notes, beats[beat], bassPitch));
        features.onset.push_back(strongestOnset(envelope, beats[beat]));
        features.chordChange.push_back(
            beat == 0 ? 1.0 : 1.0 - dot(features.chroma[beat], features.chroma[beat - 1]));
    }
    return features;
}

namespace
{

constexpr int kShortestPhrase = 4;
constexpr int kLongestPhrase = 32;
/// The shortest period that repeats almost as well as the best one: 12 rather than 24.
constexpr double kNearlyAsGood = 0.93;
constexpr double kStrongSimilarity = 0.55;
constexpr double kStrongAboveMean = 0.2;

} // namespace

Phrase phrasePeriod(const std::vector<Chroma>& chroma)
{
    const int longest = std::min(kLongestPhrase, static_cast<int>(chroma.size() / 2));
    if (longest < kShortestPhrase)
        return {};
    std::vector<double> similarity(static_cast<std::size_t>(longest) + 1);
    for (int lag = 1; lag <= longest; ++lag)
    {
        double sum = 0.0;
        for (std::size_t index = 0; index + static_cast<std::size_t>(lag) < chroma.size(); ++index)
            sum += dot(chroma[index], chroma[index + static_cast<std::size_t>(lag)]);
        similarity[static_cast<std::size_t>(lag)] =
            sum / static_cast<double>(chroma.size() - static_cast<std::size_t>(lag));
    }
    int best = kShortestPhrase;
    for (int lag = kShortestPhrase; lag <= longest; ++lag)
        if (similarity[static_cast<std::size_t>(lag)] > similarity[static_cast<std::size_t>(best)])
            best = lag;
    const double bestSimilarity = similarity[static_cast<std::size_t>(best)];
    int period = kShortestPhrase;
    while (similarity[static_cast<std::size_t>(period)] < kNearlyAsGood * bestSimilarity)
        ++period;
    const double meanSimilarity =
        std::accumulate(similarity.begin() + 1, similarity.end(), 0.0) / static_cast<double>(longest);
    const double periodSimilarity = similarity[static_cast<std::size_t>(period)];
    return {.beats = period,
            .similarity = periodSimilarity,
            .strong =
                periodSimilarity > kStrongSimilarity && periodSimilarity > meanSimilarity + kStrongAboveMean};
}

namespace
{

constexpr int kFewestBeats = 3;
constexpr int kMostBeats = 7;
/// 4/4 is the most common meter.
constexpr std::array<double, 5> kMeterPrior{-0.4, 0.0, -1.2, -0.6, -1.5};
constexpr double kPhraseBonus = 0.5;
constexpr double kChordChangeWeight = 0.35;
constexpr double kOnsetWeight = 0.2;
constexpr double kDurationalWeight = 0.3;

std::vector<double> standardised(const std::vector<double>& values)
{
    const double average = mean(values);
    const double deviation = std::sqrt(variance(values)) + kTiny;
    std::vector<double> result;
    result.reserve(values.size());
    for (const double value : values)
        result.push_back((value - average) / deviation);
    return result;
}

/// The features combined into one downbeat score per beat.
std::vector<double> downbeatScore(const BeatFeatures& features)
{
    const double total = kChordChangeWeight + kOnsetWeight + kDurationalWeight;
    const auto change = standardised(features.chordChange);
    const auto onset = standardised(features.onset);
    const auto duration = standardised(features.durational);
    std::vector<double> score(change.size());
    for (std::size_t beat = 0; beat < score.size(); ++beat)
        score[beat] = ((kChordChangeWeight * change[beat]) + (kOnsetWeight * onset[beat]) +
                       (kDurationalWeight * duration[beat])) /
                      total;
    return score;
}

struct PhaseScore
{
    double t = -1e9;
    int phase = 0;
};

/// The phase whose beats stand out most from the others (Welch's t statistic).
PhaseScore bestPhase(const std::vector<double>& score, int beatsPerBar)
{
    PhaseScore best;
    for (int phase = 0; phase < beatsPerBar; ++phase)
    {
        std::vector<double> first;
        std::vector<double> others;
        for (std::size_t beat = 0; beat < score.size(); ++beat)
            (static_cast<int>(beat) % beatsPerBar == phase ? first : others).push_back(score[beat]);
        if (first.size() < 2)
            continue;
        const double t = (mean(first) - mean(others)) /
                         std::sqrt((variance(first) / static_cast<double>(first.size())) +
                                   (variance(others) / static_cast<double>(others.size())) + kTiny);
        if (t >= best.t)
            best = {.t = t, .phase = phase};
    }
    return best;
}

} // namespace

MeterEstimate estimateMeter(const BeatFeatures& features)
{
    const auto score = downbeatScore(features);
    MeterEstimate estimate;
    estimate.phrase = phrasePeriod(features.chroma);
    std::vector<double> scores;
    for (int beatsPerBar = kFewestBeats; beatsPerBar <= kMostBeats; ++beatsPerBar)
    {
        const auto phase = bestPhase(score, beatsPerBar);
        double value = phase.t + kMeterPrior.at(static_cast<std::size_t>(beatsPerBar - kFewestBeats));
        if (estimate.phrase.strong)
            value += estimate.phrase.beats % beatsPerBar == 0 ? kPhraseBonus : -kPhraseBonus;
        scores.push_back(value);
        estimate.ranking.push_back({.beatsPerBar = beatsPerBar, .confidence = 0.0, .phase = phase.phase});
    }
    const double highest = *std::ranges::max_element(scores);
    double total = 0.0;
    for (const double value : scores)
        total += std::exp(value - highest);
    for (std::size_t index = 0; index < scores.size(); ++index)
        estimate.ranking[index].confidence = std::exp(scores[index] - highest) / total;
    std::ranges::stable_sort(estimate.ranking,
                             [](const auto& a, const auto& b) { return a.confidence > b.confidence; });
    return estimate;
}

} // namespace snotes::rhythm
