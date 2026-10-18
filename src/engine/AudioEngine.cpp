#include "engine/AudioEngine.h"

namespace snotes::engine
{

void AudioEngine::prepare(double sampleRate) noexcept
{
    parts_.tone.prepare(sampleRate);
    parts_.probe.prepare(sampleRate);
    click_.prepare(sampleRate);
}

void AudioEngine::process(
    const float* const* inputs, int numInputs, float* const* outputs, int numOutputs, int numSamples) noexcept
{
    const ProjectSnapshot* const snapshot = parts_.snapshots.acquire();
    const TransportBlock block = parts_.transport.beginBlock();
    parts_.meter.process(inputs, numInputs, numSamples);
    // Recording only runs with the transport, so its first sample has a song position.
    if (block.playing)
        parts_.recorder.process(inputs, numInputs, numSamples, block.position);
    // The tone overwrites the outputs, silence while it is off; everything else adds to them.
    parts_.tone.render(outputs, numOutputs, numSamples);
    if (snapshot != nullptr)
    {
        if (block.playing)
        {
            mixer_.renderTakes(*snapshot, block.position, outputs, numOutputs, numSamples);
            click_.render(outputs, numOutputs, numSamples, block.position, snapshot->clickBpm);
        }
        PlaybackMixer::renderMonitoring(*snapshot, inputs, numInputs, outputs, numOutputs, numSamples);
    }
    parts_.probe.process(inputs, numInputs, outputs, numOutputs, numSamples);
    parts_.transport.endBlock(block, numSamples);
}

} // namespace snotes::engine
