#pragma once

#include "engine/AudioEngine.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>
#include <cstdint>

namespace snotes::app
{

/// Humble object (E-03): forwards the driver callback to the engine::AudioEngine.
class AudioCallback final : public juce::AudioIODeviceCallback
{
public:
    explicit AudioCallback(engine::AudioEngine::Parts parts) : engine_(parts) {}

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& /*context*/) override
    {
        engine_.process(inputChannelData, numInputChannels, outputChannelData, numOutputChannels, numSamples);
        blocks_.fetch_add(1, std::memory_order_relaxed);
    }

    /// Callbacks so far; the GUI sees from it whether the driver still runs (Q-09).
    [[nodiscard]] std::uint64_t blockCount() const noexcept
    {
        return blocks_.load(std::memory_order_relaxed);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        engine_.prepare(device->getCurrentSampleRate());
    }

    void audioDeviceStopped() override {}

private:
    engine::AudioEngine engine_;
    std::atomic<std::uint64_t> blocks_{0};
};

} // namespace snotes::app
