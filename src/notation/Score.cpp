#include "notation/Score.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>

namespace snotes::notation
{
namespace
{

std::int64_t ticks(const rhythm::Fraction& beats)
{
    return std::llround(static_cast<double>(beats.numerator()) * static_cast<double>(kTicksPerBeat) /
                        static_cast<double>(beats.denominator()));
}

/// Rounds towards minus infinity; divisor is positive.
std::int64_t floorDivide(std::int64_t dividend, std::int64_t divisor)
{
    const auto quotient = dividend / divisor;
    return dividend % divisor < 0 ? quotient - 1 : quotient;
}

ScoreVoice voiceOf(const VoiceEvents& input, std::int64_t origin)
{
    ScoreVoice voice;
    voice.name = input.name;
    voice.clef = input.clef;
    voice.fretboard = input.fretboard;
    voice.events.reserve(input.events.size());
    for (std::size_t index = 0; index < input.events.size(); ++index)
    {
        const auto& event = input.events[index];
        const auto& tab = input.tabs[index];
        const auto start = ticks(event.position) - origin;
        auto end = start + std::max<std::int64_t>(1, ticks(event.duration));
        if (index + 1 < input.events.size())
            end = std::min(end, ticks(input.events[index + 1].position) - origin);
        ScoreEvent scored;
        scored.startTick = start;
        scored.durationTicks = end - start;
        scored.chord.pitches = event.pitches;
        scored.chord.shape = tab.notes;
        scored.graces.reserve(event.graces.size());
        for (std::size_t grace = 0; grace < event.graces.size(); ++grace)
            scored.graces.push_back({.pitches = event.graces[grace], .shape = tab.graces[grace]});
        voice.events.push_back(std::move(scored));
    }
    return voice;
}

std::optional<std::int64_t> firstTick(const std::vector<VoiceEvents>& voices)
{
    std::optional<std::int64_t> first;
    for (const auto& voice : voices)
        if (!voice.events.empty())
        {
            const auto tick = ticks(voice.events.front().position);
            first = std::min(first.value_or(tick), tick);
        }
    return first;
}

} // namespace

Score buildScore(const std::vector<VoiceEvents>& voices, const Meter& meter)
{
    // Without events the score starts on a downbeat.
    const auto first = firstTick(voices).value_or(meter.phase * kTicksPerBeat);
    Score score{.beatsPerBar = meter.beatsPerBar, .pickupTicks = 0, .bars = 0, .voices = {}};
    const auto bar = score.barTicks();
    const auto downbeat = meter.phase * kTicksPerBeat;
    const auto barStart = downbeat + floorDivide(first - downbeat, bar) * bar;
    const auto origin = floorDivide(first, kTicksPerBeat) * kTicksPerBeat;
    if (origin != barStart)
        score.pickupTicks = barStart + bar - origin;
    std::int64_t end = 0;
    score.voices.reserve(voices.size());
    for (const auto& input : voices)
    {
        score.voices.push_back(voiceOf(input, origin));
        for (const auto& event : score.voices.back().events)
            end = std::max(end, event.startTick + event.durationTicks);
    }
    const auto afterPickup = std::max<std::int64_t>(1, end - score.pickupTicks);
    score.bars = static_cast<int>(floorDivide(afterPickup + bar - 1, bar));
    return score;
}

} // namespace snotes::notation
