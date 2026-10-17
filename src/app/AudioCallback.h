#pragma once

#include "engine/InputMeter.h"
#include "engine/Recorder.h"
#include "engine/TestToneSource.h"

#include <juce_audio_devices/juce_audio_devices.h>

#include <atomic>
#include <cstdint>

namespace snotes::app
{

/// Humble object (E-03): forwards the driver callback to the input meters, the recorder and the test tone.
/// The playback of the voices (AP3) joins here.
class AudioCallback final : public juce::AudioIODeviceCallback
{
public:
    AudioCallback(engine::TestToneSource& tone, engine::InputMeter& meter, engine::Recorder& recorder)
        : tone_(tone), meter_(meter), recorder_(recorder)
    {
    }

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& /*context*/) override
    {
        meter_.process(inputChannelData, numInputChannels, numSamples);
        recorder_.process(inputChannelData, numInputChannels, numSamples);
        // The tone overwrites the outputs, silence while it is off.
        tone_.render(outputChannelData, numOutputChannels, numSamples);
        blocks_.fetch_add(1, std::memory_order_relaxed);
    }

    /// Callbacks so far; the GUI sees from it whether the driver still runs (Q-09).
    [[nodiscard]] std::uint64_t blockCount() const noexcept
    {
        return blocks_.load(std::memory_order_relaxed);
    }

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override
    {
        tone_.prepare(device->getCurrentSampleRate());
    }

    void audioDeviceStopped() override {}

private:
    engine::TestToneSource& tone_;
    engine::InputMeter& meter_;
    engine::Recorder& recorder_;
    std::atomic<std::uint64_t> blocks_{0};
};

} // namespace snotes::app
