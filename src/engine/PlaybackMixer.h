#pragma once

#include "engine/ProjectSnapshot.h"

#include <array>
#include <cstdint>

namespace snotes::engine
{

/// Mixes the voices into the outputs on the audio thread (F-WI-02, F-WI-03, F-WI-06): the active take of
/// every audible voice at its place in the song, and the inputs of the monitored voices. Every voice
/// plays on all outputs. No allocation, no lock (Q-04).
class PlaybackMixer
{
public:
    static constexpr int kChunk = 512;

    /// Adds the takes from song position `position` on. A take recorded from song sample S with latency L
    /// plays from S - L: what was played to the heard playback lands where it was heard (F-WI-05).
    void renderTakes(const ProjectSnapshot& snapshot,
                     std::int64_t position,
                     float* const* outputs,
                     int numOutputs,
                     int numSamples) noexcept;
    /// Adds the input of every monitored voice: processed if routed, else DI.
    static void renderMonitoring(const ProjectSnapshot& snapshot,
                                 const float* const* inputs,
                                 int numInputs,
                                 float* const* outputs,
                                 int numOutputs,
                                 int numSamples) noexcept;

private:
    static void add(const float* source,
                    float gain,
                    float* const* outputs,
                    int numOutputs,
                    int offset,
                    int count) noexcept;

    std::array<float, kChunk> scratch_{};
};

} // namespace snotes::engine
