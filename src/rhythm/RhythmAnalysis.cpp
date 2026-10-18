#include "rhythm/RhythmAnalysis.h"

#include "rhythm/OnsetStrength.h"
#include "rhythm/Statistics.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>

namespace snotes::rhythm
{

Onsets measureOnsets(std::span<const float> audio)
{
    return {.beat = onsetStrength(audio, kBeatHop), .detail = onsetStrength(audio, kDetailHop)};
}

namespace
{

/// The beats before the first note are kept from 0.6 beats on, after the last start for one beat.
constexpr double kLeadBeats = 0.6;
constexpr double kTailBeats = 1.0;

/// The envelopes of all tracks added up in song time.
std::vector<float>
sumInSongTime(const std::vector<RhythmTrack>& tracks, int hop, std::vector<float> Onsets::*envelope)
{
    std::vector<float> sum;
    for (const auto& track : tracks)
    {
        const auto& values = track.onsets.*envelope;
        const auto offset = static_cast<std::size_t>(std::lround(track.startSeconds * kAnalysisRate / hop));
        sum.resize(std::max(sum.size(), offset + values.size()));
        for (std::size_t frame = 0; frame < values.size(); ++frame)
            sum[offset + frame] += values[frame];
    }
    return sum;
}

std::vector<transcription::Note> notesInSongTime(const std::vector<RhythmTrack>& tracks)
{
    std::vector<transcription::Note> notes;
    for (const auto& track : tracks)
        for (auto note : track.notes)
        {
            note.startSeconds += track.startSeconds;
            note.endSeconds += track.startSeconds;
            notes.push_back(note);
        }
    std::ranges::stable_sort(notes,
                             [](const auto& a, const auto& b) {
                                 return a.startSeconds < b.startSeconds ||
                                        (a.startSeconds == b.startSeconds && a.pitch < b.pitch);
                             });
    return notes;
}

std::vector<double> aroundTheNotes(const std::vector<double>& beats,
                                   const std::vector<transcription::Note>& notes)
{
    std::vector<double> intervals;
    for (std::size_t beat = 1; beat < beats.size(); ++beat)
        intervals.push_back(beats[beat] - beats[beat - 1]);
    const double interval = median(intervals);
    const double lastStart = std::ranges::max(notes, {}, &transcription::Note::startSeconds).startSeconds;
    std::vector<double> kept;
    std::ranges::copy_if(beats,
                         std::back_inserter(kept),
                         [&](double beat)
                         {
                             return beat >= notes.front().startSeconds - (kLeadBeats * interval) &&
                                    beat <= lastStart + (kTailBeats * interval);
                         });
    return kept;
}

} // namespace

std::optional<BeatGrid> commonBeatGrid(const std::vector<RhythmTrack>& tracks,
                                       const BeatTrackerSettings& settings)
{
    const auto notes = notesInSongTime(tracks);
    if (notes.empty())
        return std::nullopt;
    const double framesPerSecond = kAnalysisRate / kBeatHop;
    const auto envelope = sumInSongTime(tracks, kBeatHop, &Onsets::beat);
    BeatGrid grid;
    grid.tempoCandidates = tempoCandidates(envelope, framesPerSecond);
    if (grid.tempoCandidates.empty())
        return std::nullopt;
    const auto beats = trackBeats(envelope, framesPerSecond, grid.tempoCandidates.front().bpm, settings);
    if (beats.size() < 2)
        return std::nullopt;
    grid.beats = aroundTheNotes(beats, notes);
    if (grid.beats.size() < 2)
        return std::nullopt;
    grid.meter =
        estimateMeter(beatFeatures(grid.beats, notes, sumInSongTime(tracks, kDetailHop, &Onsets::detail)));
    return grid;
}

std::vector<double> tempoCurve(const std::vector<double>& beats)
{
    std::vector<double> tempo;
    for (std::size_t beat = 1; beat < beats.size(); ++beat)
        tempo.push_back(60.0 / (beats[beat] - beats[beat - 1]));
    return tempo;
}

VoiceRhythm voiceRhythm(const RhythmTrack& track, const BeatGrid& grid)
{
    std::vector<double> beats;
    beats.reserve(grid.beats.size());
    for (const double beat : grid.beats)
        beats.push_back(beat - track.startSeconds);
    VoiceRhythm rhythm{.style = guessStyle(track.notes), .quantization = {}};
    const auto rules = rulesFor(rhythm.style.style);
    rhythm.quantization = quantize(
        cleanUp(clusterOnsets(track.notes, rules.relativeAmplitude), track.onsets.detail, beats, rules),
        beats);
    return rhythm;
}

} // namespace snotes::rhythm
