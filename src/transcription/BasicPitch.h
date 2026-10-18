#pragma once

#include <cstddef>
#include <vector>

/// Constants of the Basic Pitch model (Spotify, ICASSP 2022, Apache 2.0), version 0.4.0, which the note
/// detection runs (F-TX-02). The names follow basic_pitch/constants.py so the port can be compared line by
/// line (E-12).
namespace snotes::transcription::basicpitch
{

inline constexpr double kSampleRate = 22050.0;
inline constexpr int kFftHop = 256;
/// ANNOTATIONS_FPS: integer division in Python, 86 frames per second.
inline constexpr int kFramesPerSecond = 86;
/// One model window: two seconds minus one hop.
inline constexpr int kWindowSamples = 43844;
inline constexpr int kFramesPerWindow = 172;
/// Neighbouring windows overlap by 30 frames; half of them is dropped on each side.
inline constexpr int kOverlapFrames = 30;
inline constexpr int kOverlapSamples = kOverlapFrames * kFftHop;
inline constexpr int kWindowHop = kWindowSamples - kOverlapSamples;
inline constexpr int kNumPitches = 88;
/// MIDI note of the lowest pitch bin, A0.
inline constexpr int kLowestPitch = 21;

} // namespace snotes::transcription::basicpitch

namespace snotes::transcription
{

/// Output of the model over a whole recording: per frame and pitch the probability that a note sounds
/// ("note") and that one starts ("onset").
struct Activations
{
    int numFrames = 0;
    /// numFrames x kNumPitches, frame-major.
    std::vector<float> frames;
    std::vector<float> onsets;

    [[nodiscard]] float frame(int frameIndex, int pitchIndex) const
    {
        return frames[index(frameIndex, pitchIndex)];
    }

    [[nodiscard]] float onset(int frameIndex, int pitchIndex) const
    {
        return onsets[index(frameIndex, pitchIndex)];
    }

    [[nodiscard]] static std::size_t index(int frameIndex, int pitchIndex)
    {
        return (static_cast<std::size_t>(frameIndex) * basicpitch::kNumPitches) +
               static_cast<std::size_t>(pitchIndex);
    }
};

} // namespace snotes::transcription
