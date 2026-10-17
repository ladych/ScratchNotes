#include "engine/RecordingSession.h"

#include <span>

namespace snotes::engine
{

RecordingSession::RecordingSession(const std::vector<int>& inputs, std::size_t ringCapacity) : inputs_(inputs)
{
    for (std::size_t channel = 0; channel < inputs_.size(); ++channel)
        channels_.push_back(std::make_unique<AudioRingBuffer>(ringCapacity));
}

void RecordingSession::record(const float* const* inputs,
                              int numInputs,
                              int numSamples,
                              std::int64_t position) noexcept
{
    if (start_.load() == kNotStarted)
        start_.store(position);
    const auto count = static_cast<std::size_t>(numSamples);
    const std::span deviceInputs{inputs, static_cast<std::size_t>(numInputs)};
    std::int64_t dropped = 0;
    for (std::size_t channel = 0; channel < channels_.size(); ++channel)
    {
        const int input = inputs_[channel];
        const float* samples =
            input >= 0 && input < numInputs ? deviceInputs[static_cast<std::size_t>(input)] : nullptr;
        const auto stored = samples != nullptr ? channels_[channel]->write(samples, count)
                                               : channels_[channel]->writeSilence(count);
        dropped += static_cast<std::int64_t>(count - stored);
    }
    recorded_.fetch_add(numSamples);
    if (dropped > 0)
        dropped_.fetch_add(dropped);
}

} // namespace snotes::engine
