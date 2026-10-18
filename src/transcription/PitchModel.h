#pragma once

#include <span>

namespace snotes::transcription
{

/// The neural network of the note detection, one window at a time (F-TX-02).
class IPitchModel
{
public:
    virtual ~IPitchModel() = default;

    /// Analyses basicpitch::kWindowSamples samples at 22.05 kHz and writes kFramesPerWindow x kNumPitches
    /// note and onset activations.
    virtual void run(std::span<const float> window, std::span<float> frames, std::span<float> onsets) = 0;
};

} // namespace snotes::transcription
