#include "transcription/PitchAnalysis.h"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace snotes::transcription
{
namespace
{

using namespace basicpitch;

constexpr std::size_t kCellsPerWindow = static_cast<std::size_t>(kFramesPerWindow) * kNumPitches;
/// Half of the overlap is dropped at each end of a window.
constexpr int kDroppedFrames = kOverlapFrames / 2;
constexpr int kKeptFrames = kFramesPerWindow - (2 * kDroppedFrames);

/// The audio with half an overlap of silence in front, as get_audio_input pads it.
std::vector<float> padded(std::span<const float> audio)
{
    std::vector<float> result(static_cast<std::size_t>(kOverlapSamples / 2), 0.0F);
    result.insert(result.end(), audio.begin(), audio.end());
    return result;
}

/// Appends the frames of one window without its overlapping ends.
void append(const std::vector<float>& window, std::vector<float>& target)
{
    const auto first = window.begin() + static_cast<std::ptrdiff_t>(kDroppedFrames) * kNumPitches;
    target.insert(target.end(), first, first + static_cast<std::ptrdiff_t>(kKeptFrames) * kNumPitches);
}

} // namespace

std::optional<Activations>
analysePitches(std::span<const float> audio, IPitchModel& model, const ProgressCallback& progress)
{
    const auto input = padded(audio);
    const std::size_t numWindows = (input.size() + kWindowHop - 1) / kWindowHop;
    std::vector<float> window(kWindowSamples);
    std::vector<float> frames(kCellsPerWindow);
    std::vector<float> onsets(kCellsPerWindow);
    Activations result;
    for (std::size_t index = 0; index < numWindows; ++index)
    {
        if (!progress(static_cast<double>(index) / static_cast<double>(numWindows)))
            return std::nullopt;
        const std::size_t start = index * kWindowHop;
        const std::size_t count = std::min<std::size_t>(kWindowSamples, input.size() - start);
        std::fill(std::copy_n(input.begin() + static_cast<std::ptrdiff_t>(start), count, window.begin()),
                  window.end(),
                  0.0F);
        model.run(window, frames, onsets);
        append(frames, result.frames);
        append(onsets, result.onsets);
    }
    // unwrap_output trims to the length of the audio; ANNOTATIONS_FPS is an integer there.
    result.numFrames =
        static_cast<int>((audio.size() * kFramesPerSecond) / static_cast<std::size_t>(kSampleRate));
    result.frames.resize(Activations::index(result.numFrames, 0));
    result.onsets.resize(result.frames.size());
    return result;
}

} // namespace snotes::transcription
