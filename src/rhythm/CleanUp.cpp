#include "rhythm/CleanUp.h"

#include "rhythm/OnsetStrength.h"
#include "rhythm/Statistics.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <numeric>
#include <ranges>
#include <set>
#include <utility>

namespace snotes::rhythm
{
namespace
{

constexpr double kClusterTolerance = 0.07;
constexpr double kEndPercentile = 75.0;
constexpr double kDefaultRelativeAmplitude = 0.5;
constexpr double kChordShareForRiff = 0.5;

/// PoC _finish_cluster: time, end, pitches and weight from the notes.
void finish(Cluster& cluster, double relativeAmplitude)
{
    std::vector<double> starts;
    std::vector<double> ends;
    float loudest = 0.0F;
    cluster.weight = 0.0;
    for (const auto& note : cluster.notes)
    {
        starts.push_back(note.startSeconds);
        ends.push_back(note.endSeconds);
        loudest = std::max(loudest, note.amplitude);
        cluster.weight += note.amplitude * (note.endSeconds - note.startSeconds);
    }
    cluster.seconds = median(starts);
    cluster.endSeconds = percentile(ends, kEndPercentile);
    std::set<int> pitches;
    for (const auto& note : cluster.notes)
        if (note.amplitude >= relativeAmplitude * loudest)
            pitches.insert(note.pitch);
    cluster.pitches.assign(pitches.begin(), pitches.end());
}

} // namespace

std::vector<Cluster> clusterOnsets(const std::vector<transcription::Note>& notes, double relativeAmplitude)
{
    std::vector<Cluster> clusters;
    for (const auto& note : notes)
    {
        if (clusters.empty() || note.startSeconds - clusters.back().firstSeconds >= kClusterTolerance)
            clusters.push_back({.firstSeconds = note.startSeconds});
        clusters.back().notes.push_back(note);
    }
    for (auto& cluster : clusters)
        finish(cluster, relativeAmplitude);
    return clusters;
}

StyleGuess guessStyle(const std::vector<transcription::Note>& notes)
{
    const auto clusters = clusterOnsets(notes, kDefaultRelativeAmplitude);
    if (clusters.empty())
        return {};
    const auto chords =
        std::ranges::count_if(clusters, [](const Cluster& cluster) { return cluster.pitches.size() >= 2; });
    const double share = static_cast<double>(chords) / static_cast<double>(clusters.size());
    return {.style = share >= kChordShareForRiff ? Style::riff : Style::melody, .chordShare = share};
}

CleanUpRules rulesFor(Style style)
{
    if (style == Style::riff)
        return {};
    return {.mergeLateNotes = false, .graceBeats = 0.15, .graceWeightRatio = 0.5};
}

std::vector<std::size_t> pickPeaks(std::span<const float> envelope, const PeakPicking& settings)
{
    const auto size = static_cast<std::ptrdiff_t>(envelope.size());
    const auto value = [&envelope](std::ptrdiff_t frame)
    { return envelope[static_cast<std::size_t>(frame)]; };
    const auto maximum = [&](std::ptrdiff_t from, std::ptrdiff_t to)
    {
        return *std::max_element(envelope.begin() + std::max<std::ptrdiff_t>(0, from),
                                 envelope.begin() + std::min(to, size));
    };
    const auto average = [&](std::ptrdiff_t from, std::ptrdiff_t to)
    {
        const auto first = envelope.begin() + std::max<std::ptrdiff_t>(0, from);
        const auto last = envelope.begin() + std::min(to, size);
        return std::accumulate(first, last, 0.0) / static_cast<double>(std::distance(first, last));
    };
    std::vector<std::size_t> peaks;
    std::ptrdiff_t frame = 0;
    while (frame < size)
    {
        const bool isMaximum = value(frame) >= maximum(frame - settings.preMax, frame + settings.postMax);
        if (isMaximum && value(frame) >= average(frame - settings.preAverage, frame + settings.postAverage) +
                                             settings.delta)
        {
            peaks.push_back(static_cast<std::size_t>(frame));
            frame += settings.wait + 1;
        }
        else
            ++frame;
    }
    return peaks;
}

namespace
{

constexpr double kFluxWindow = 0.04;
constexpr double kReferencePercentile = 75.0;
constexpr double kStrongAttack = 0.6;
constexpr double kNoAttack = 0.15;
constexpr double kStillRinging = 0.05;
constexpr double kLateNote = 0.2;
constexpr int kLowResonance = 5;

/// The typical onset: the 75th percentile of the peaks, the strongest onset if there are none.
double typicalOnset(std::span<const float> envelope)
{
    std::vector<double> peaks;
    for (const auto frame : pickPeaks(envelope, PeakPicking{}))
        peaks.push_back(envelope[frame]);
    if (peaks.empty())
        return *std::ranges::max_element(envelope);
    return percentile(std::move(peaks), kReferencePercentile);
}

/// Attack strength around each cluster relative to the typical onset.
void measureFlux(std::vector<Cluster>& clusters, std::span<const float> envelope)
{
    // Without onsets every cluster counts as having no attack.
    if (envelope.empty())
        return;
    const double reference = typicalOnset(envelope);
    for (auto& cluster : clusters)
    {
        double strongest = -1.0;
        for (std::size_t frame = 0; frame < envelope.size(); ++frame)
        {
            const double seconds = frameSeconds(frame, kDetailHop);
            if (seconds > cluster.seconds - kFluxWindow && seconds < cluster.seconds + kFluxWindow)
                strongest = std::max(strongest, static_cast<double>(envelope[frame]));
        }
        cluster.flux = strongest < 0.0 ? 0.0 : strongest / reference;
    }
}

std::set<int> pitchClasses(const std::vector<int>& pitches)
{
    std::set<int> classes;
    for (const int pitch : pitches)
        classes.insert(pitch % 12);
    return classes;
}

/// Whether the cluster without attack after the previous kept one is noise; merges it if the rules say so.
bool absorbedByPrevious(Cluster& cluster, Cluster& previous, const CleanUpRules& rules)
{
    const auto classes = pitchClasses(cluster.pitches);
    const auto ringing = pitchClasses(previous.pitches);
    const bool stillRinging = previous.endSeconds > cluster.seconds - kStillRinging;
    if (stillRinging && std::ranges::includes(ringing, classes))
        return true;
    if (cluster.seconds - previous.seconds >= kLateNote)
        return false;
    if (rules.mergeLateNotes)
    {
        previous.notes.insert(previous.notes.end(), cluster.notes.begin(), cluster.notes.end());
        finish(previous, rules.relativeAmplitude);
        previous.seconds = previous.firstSeconds;
        return true;
    }
    return cluster.pitches.back() <= previous.pitches.front() - kLowResonance;
}

/// Whether every pitch of the cluster lies below a note of a kept cluster that rings on past it, or shares
/// the pitch class of a note that rings or just stopped (as at a window seam of the model). A note that stops
/// as the cluster starts is left alone: that is a pull-off or a new note, not a ghost.
bool underRinging(const Cluster& cluster, const std::vector<Cluster>& kept)
{
    int highest = std::numeric_limits<int>::min();
    std::set<int> classes;
    for (const auto& earlier : kept)
        for (const auto& note : earlier.notes)
        {
            if (note.endSeconds > cluster.seconds + kStillRinging)
                highest = std::max(highest, note.pitch);
            if (note.endSeconds > cluster.seconds - kStillRinging)
                classes.insert(note.pitch % 12);
        }
    return std::ranges::all_of(cluster.pitches,
                               [highest, &classes](int pitch)
                               { return pitch < highest || classes.contains(pitch % 12); });
}

/// Light clusters shortly before a heavier one become its grace notes.
std::vector<Cluster> withGraceNotes(std::vector<Cluster> kept, double beatSeconds, const CleanUpRules& rules)
{
    std::vector<Cluster> events;
    for (auto& cluster : std::views::reverse(kept))
    {
        if (!events.empty() && events.back().seconds - cluster.seconds < rules.graceBeats * beatSeconds &&
            cluster.weight < rules.graceWeightRatio * events.back().weight)
        {
            const auto count = std::min<std::size_t>(2, cluster.pitches.size());
            events.back().graces.insert(
                events.back().graces.begin(),
                std::vector<int>(cluster.pitches.begin(),
                                 cluster.pitches.begin() + static_cast<std::ptrdiff_t>(count)));
            continue;
        }
        events.push_back(std::move(cluster));
    }
    std::ranges::reverse(events);
    return events;
}

} // namespace

std::vector<Cluster> cleanUp(std::vector<Cluster> clusters,
                             std::span<const float> envelope,
                             std::span<const double> beats,
                             const CleanUpRules& rules)
{
    measureFlux(clusters, envelope);
    std::vector<Cluster> kept;
    for (auto& cluster : clusters)
    {
        if (cluster.weight < rules.dropWeight ||
            (cluster.weight < rules.dropSoftWeight && cluster.flux < kStrongAttack))
            continue;
        if (!kept.empty() && cluster.flux < kNoAttack && absorbedByPrevious(cluster, kept.back(), rules))
            continue;
        if (rules.dropGhostsUnderRinging && cluster.flux < kNoAttack && underRinging(cluster, kept))
            continue;
        kept.push_back(std::move(cluster));
    }
    std::vector<double> intervals;
    for (std::size_t beat = 1; beat < beats.size(); ++beat)
        intervals.push_back(beats[beat] - beats[beat - 1]);
    return withGraceNotes(std::move(kept), median(intervals), rules);
}

} // namespace snotes::rhythm
