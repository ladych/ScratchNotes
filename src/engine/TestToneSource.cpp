#include "engine/TestToneSource.h"

#include <cmath>
#include <numbers>

namespace snotes::engine
{

TestToneSource::TestToneSource(double frequencyHz, float gain) noexcept
    : frequencyHz_(frequencyHz), gain_(gain)
{
}

void TestToneSource::prepare(double sampleRate) noexcept
{
    phase_ = 0.0;
    phaseIncrement_ = frequencyHz_ / sampleRate;
}

void TestToneSource::setEnabled(bool enabled) noexcept
{
    enabled_.store(enabled, std::memory_order_relaxed);
}

bool TestToneSource::isEnabled() const noexcept
{
    return enabled_.load(std::memory_order_relaxed);
}

void TestToneSource::render(float* const* channels, int numChannels, int numSamples) noexcept
{
    const bool enabled = isEnabled();

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const float value = enabled ? nextSample() : 0.0F;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            float* const output = channels[channel];
            if (output != nullptr)
                output[sampleIndex] = value;
        }
    }
}

float TestToneSource::nextSample() noexcept
{
    const auto value = gain_ * static_cast<float>(std::sin(2.0 * std::numbers::pi * phase_));
    phase_ += phaseIncrement_;
    if (phase_ >= 1.0)
        phase_ -= 1.0;
    return value;
}

} // namespace snotes::engine
