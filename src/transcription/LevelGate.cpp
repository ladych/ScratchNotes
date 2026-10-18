#include "transcription/LevelGate.h"

#include "transcription/BasicPitch.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace snotes::transcription
{
namespace
{

// The PoC measured 2048 samples every 512 at 44.1 kHz: the same times at 22.05 kHz.
constexpr std::ptrdiff_t kFrameLength = 1024;
constexpr std::ptrdiff_t kHop = 256;
/// The level is read three frames after the start of the note, when the attack has arrived.
constexpr int kFramesAfterStart = 3;
constexpr double kFloor = 1e-9;

/// librosa.feature.rms with centred frames, in dB.
std::vector<double> levels(std::span<const float> audio)
{
    const auto size = static_cast<std::ptrdiff_t>(audio.size());
    std::vector<double> result(static_cast<std::size_t>((size / kHop) + 1));
    for (std::size_t frame = 0; frame < result.size(); ++frame)
    {
        const std::ptrdiff_t centre = static_cast<std::ptrdiff_t>(frame) * kHop;
        double power = 0.0;
        for (auto sample = std::max<std::ptrdiff_t>(0, centre - (kFrameLength / 2));
             sample < std::min(size, centre + (kFrameLength / 2));
             ++sample)
            power += static_cast<double>(audio[static_cast<std::size_t>(sample)]) *
                     audio[static_cast<std::size_t>(sample)];
        result[frame] = 20.0 * std::log10(std::sqrt(power / kFrameLength) + kFloor);
    }
    return result;
}

} // namespace

std::vector<Note>
applyLevelGate(const std::vector<Note>& notes, std::span<const float> audio, const GateSettings& settings)
{
    const auto level = levels(audio);
    const double threshold = *std::ranges::max_element(level) - settings.rangeDb;
    const auto last = static_cast<std::ptrdiff_t>(level.size()) - 1;
    std::vector<Note> kept;
    for (const auto& note : notes)
    {
        const auto frame = static_cast<std::ptrdiff_t>((note.startSeconds * basicpitch::kSampleRate / kHop) +
                                                       kFramesAfterStart);
        if (level[static_cast<std::size_t>(std::clamp<std::ptrdiff_t>(frame, 0, last))] > threshold &&
            note.amplitude >= settings.minimumAmplitude)
            kept.push_back(note);
    }
    return kept;
}

} // namespace snotes::transcription
