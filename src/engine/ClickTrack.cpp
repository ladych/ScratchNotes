#include "engine/ClickTrack.h"

#include <cmath>
#include <numbers>

namespace snotes::engine
{

void ClickTrack::prepare(double sampleRate) noexcept
{
    sampleRate_ = sampleRate;
    length_ = std::llround(kClickSeconds * sampleRate);
}

void ClickTrack::render(float* const* outputs,
                        int numOutputs,
                        int numSamples,
                        std::int64_t position,
                        double beatsPerMinute) const noexcept
{
    if (beatsPerMinute <= 0.0)
        return;
    const double beatLength = sampleRate_ * 60.0 / beatsPerMinute;
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const std::int64_t songSample = position + sample;
        const auto beat = static_cast<std::int64_t>(std::floor(static_cast<double>(songSample) / beatLength));
        const std::int64_t offset = songSample - std::llround(static_cast<double>(beat) * beatLength);
        const float value = sampleAt(offset, beat % kBeatsPerBar == 0);
        for (int channel = 0; channel < numOutputs; ++channel)
            if (outputs[channel] != nullptr)
                outputs[channel][sample] += value;
    }
}

float ClickTrack::sampleAt(std::int64_t offset, bool accent) const noexcept
{
    if (offset < 0 || offset >= length_)
        return 0.0F;
    const double envelope = static_cast<double>(length_ - offset) / static_cast<double>(length_);
    const double frequency = accent ? kAccentHz : kClickHz;
    const double phase = 2.0 * std::numbers::pi * frequency * static_cast<double>(offset) / sampleRate_;
    return static_cast<float>(kLevel * envelope * std::sin(phase));
}

} // namespace snotes::engine
