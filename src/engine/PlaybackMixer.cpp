#include "engine/PlaybackMixer.h"

#include <algorithm>

namespace snotes::engine
{

void PlaybackMixer::renderTakes(const ProjectSnapshot& snapshot,
                                std::int64_t position,
                                float* const* outputs,
                                int numOutputs,
                                int numSamples) noexcept
{
    for (const auto& voice : snapshot.voices)
    {
        if (!voice.audible || !voice.audio)
            continue;
        const std::int64_t start = voice.takeStartSample - voice.takeLatencyOffsetSamples;
        const float gain = voice.gain * snapshot.masterGain;
        for (int offset = 0; offset < numSamples; offset += kChunk)
        {
            const int count = std::min(kChunk, numSamples - offset);
            voice.audio->read(position + offset - start, count, scratch_.data());
            add(scratch_.data(), gain, outputs, numOutputs, offset, count);
        }
    }
}

void PlaybackMixer::renderMonitoring(const ProjectSnapshot& snapshot,
                                     const float* const* inputs,
                                     int numInputs,
                                     float* const* outputs,
                                     int numOutputs,
                                     int numSamples) noexcept
{
    for (const auto& voice : snapshot.voices)
    {
        if (!voice.monitored)
            continue;
        const int input = voice.inputWet >= 0 ? voice.inputWet : voice.inputDi;
        if (input < 0 || input >= numInputs || inputs[input] == nullptr)
            continue;
        add(inputs[input], voice.gain * snapshot.masterGain, outputs, numOutputs, 0, numSamples);
    }
}

void PlaybackMixer::add(
    const float* source, float gain, float* const* outputs, int numOutputs, int offset, int count) noexcept
{
    for (int channel = 0; channel < numOutputs; ++channel)
    {
        float* const output = outputs[channel];
        if (output == nullptr)
            continue;
        for (int sample = 0; sample < count; ++sample)
            output[offset + sample] += gain * source[sample];
    }
}

} // namespace snotes::engine
