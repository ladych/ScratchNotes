#include "engine/NoteSynth.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace snotes::engine
{

void NoteSynth::render(const SynthNotes& notes,
                       double sampleRate,
                       std::int64_t position,
                       float gain,
                       float* const* outputs,
                       int numOutputs,
                       int numSamples) noexcept
{
    const auto release = static_cast<std::int64_t>(std::ceil(kReleaseSeconds * sampleRate));
    const auto blockEnd = position + numSamples;
    // No note that starts earlier still sounds.
    const auto earliest = position - notes.longest - release;
    auto note = std::ranges::lower_bound(notes.notes, earliest, {}, &SynthNote::start);
    for (; note != notes.notes.end() && note->start < blockEnd; ++note)
    {
        const auto to = std::min(blockEnd, note->end + release);
        for (auto sample = std::max(position, note->start); sample < to; ++sample)
        {
            const float value = gain * sampleAt(*note, sample, sampleRate);
            for (int channel = 0; channel < numOutputs; ++channel)
                if (outputs[channel] != nullptr)
                    outputs[channel][sample - position] += value;
        }
    }
}

float NoteSynth::sampleAt(const SynthNote& note, std::int64_t sample, double sampleRate) noexcept
{
    const double time = static_cast<double>(sample - note.start) / sampleRate;
    const double released = static_cast<double>(sample - note.end) / sampleRate;
    if (time < 0.0 || released >= kReleaseSeconds)
        return 0.0F;
    double envelope = std::min(1.0, time / kAttackSeconds) * std::exp(-time / kDecaySeconds);
    if (released > 0.0)
        envelope *= 1.0 - (released / kReleaseSeconds);
    const double phase = 2.0 * std::numbers::pi * note.frequency * time;
    const double tone = std::sin(phase) + (0.5 * std::sin(2.0 * phase)) + (0.25 * std::sin(3.0 * phase));
    return static_cast<float>(kLevel * envelope * tone / 1.75);
}

} // namespace snotes::engine
