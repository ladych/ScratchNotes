#pragma once

#include <cstdint>
#include <vector>

namespace snotes::engine
{

/// What the audio thread needs of one voice. Built on the GUI thread and read-only afterwards.
struct VoiceSnapshot
{
    /// Input channels, -1 for none.
    int inputDi = -1;
    int inputWet = -1;
    float gain = 1.0F;
    /// False if muted, or if another voice is solo and this one is not.
    bool audible = true;
    bool monitored = false;
    bool hasTake = false;
    std::int64_t takeStartSample = 0;
    std::int64_t takeLatencyOffsetSamples = 0;
};

/// Immutable state of the project for the audio thread (Pflichtenheft chapter 4), handed over through
/// SnapshotExchange.
struct ProjectSnapshot
{
    double sampleRate = 0.0;
    float masterGain = 1.0F;
    std::vector<VoiceSnapshot> voices;
};

} // namespace snotes::engine
