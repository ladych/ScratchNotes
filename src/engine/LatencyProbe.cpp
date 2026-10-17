#include "engine/LatencyProbe.h"

#include <cmath>
#include <cstddef>

namespace snotes::engine
{

void LatencyProbe::start(int input) noexcept
{
    input_.store(input);
    finished_.store(0);
    restart_.store(true);
    active_.store(true);
}

std::int64_t LatencyProbe::result(int index) const noexcept
{
    return results_.at(static_cast<std::size_t>(index)).load();
}

void LatencyProbe::prepare(double sampleRate) noexcept
{
    window_ = std::llround(kWindowSeconds * sampleRate);
}

void LatencyProbe::process(
    const float* const* inputs, int numInputs, float* const* outputs, int numOutputs, int numSamples) noexcept
{
    if (!active_.load())
        return;
    if (restart_.exchange(false))
    {
        elapsed_ = 0;
        peak_ = 0.0F;
        peakAt_ = kNoPeak;
    }
    const int input = input_.load();
    const float* const samples = input >= 0 && input < numInputs ? inputs[input] : nullptr;
    for (int sample = 0; sample < numSamples && active_.load(); ++sample)
    {
        if (elapsed_ == 0)
            for (int channel = 0; channel < numOutputs; ++channel)
                if (outputs[channel] != nullptr)
                    outputs[channel][sample] += kPingLevel;
        scan(samples, sample);
        if (++elapsed_ == window_)
            finishPing();
    }
}

void LatencyProbe::scan(const float* input, int sample) noexcept
{
    if (input == nullptr)
        return;
    const float level = std::abs(input[sample]);
    if (level > peak_)
    {
        peak_ = level;
        peakAt_ = elapsed_;
    }
}

void LatencyProbe::finishPing() noexcept
{
    const int ping = finished_.load();
    results_.at(static_cast<std::size_t>(ping)).store(peak_ >= kMinPeak ? peakAt_ : kNoPeak);
    finished_.store(ping + 1);
    elapsed_ = 0;
    peak_ = 0.0F;
    peakAt_ = kNoPeak;
    if (ping + 1 == kPings)
        active_.store(false);
}

} // namespace snotes::engine
