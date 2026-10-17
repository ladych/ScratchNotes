#pragma once

#include <cstdint>

namespace snotes::engine
{

/// Optional click with a fixed tempo (F-WI-07): a short sine burst on every beat, higher on the first
/// beat of a 4/4 bar. Derived from the song position alone, so it stays on the beat after every locate.
/// Audio thread only, no allocation (Q-04).
class ClickTrack
{
public:
    static constexpr double kClickHz = 1000.0;
    static constexpr double kAccentHz = 1600.0;
    static constexpr double kClickSeconds = 0.03;
    static constexpr float kLevel = 0.5F;
    static constexpr int kBeatsPerBar = 4;

    void prepare(double sampleRate) noexcept;
    /// Adds the click from song position `position` on to all outputs; nothing for a tempo <= 0.
    void render(float* const* outputs,
                int numOutputs,
                int numSamples,
                std::int64_t position,
                double beatsPerMinute) const noexcept;

    /// The click's value `offset` samples after the start of a beat.
    [[nodiscard]] float sampleAt(std::int64_t offset, bool accent) const noexcept;

private:
    double sampleRate_ = 48000.0;
    std::int64_t length_ = 0;
};

} // namespace snotes::engine
