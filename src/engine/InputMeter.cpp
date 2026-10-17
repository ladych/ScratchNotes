#include "engine/InputMeter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>

namespace snotes::engine
{

void InputMeter::process(const float* const* inputs, int numInputs, int numSamples) noexcept
{
    const std::span channels{inputs, static_cast<std::size_t>(std::min(numInputs, kMaxInputs))};
    for (std::size_t input = 0; input < channels.size(); ++input)
    {
        if (channels[input] == nullptr)
            continue;
        float blockPeak = 0.0F;
        for (const float sample : std::span{channels[input], static_cast<std::size_t>(numSamples)})
            blockPeak = std::max(blockPeak, std::abs(sample));
        // Only this thread raises the peak; a reset by the GUI thread in between merely shows this block's
        // peak once more.
        auto& peak = peaks_.at(input);
        if (blockPeak > peak.load(std::memory_order_relaxed))
            peak.store(blockPeak, std::memory_order_relaxed);
    }
}

float InputMeter::takePeak(int input) noexcept
{
    if (input < 0 || input >= kMaxInputs)
        return 0.0F;
    return peaks_.at(static_cast<std::size_t>(input)).exchange(0.0F, std::memory_order_relaxed);
}

} // namespace snotes::engine
