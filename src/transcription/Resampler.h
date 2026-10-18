#pragma once

#include <span>
#include <vector>

namespace snotes::transcription
{

/// Converts audio to another sample rate with a windowed-sinc low-pass (Kaiser window), so that nothing
/// above the new Nyquist frequency folds back. The model needs 22.05 kHz; recordings are taken at 44.1 to
/// 192 kHz (F-TX-02). The result has ceil(n * toRate / fromRate) samples, as librosa.resample.
[[nodiscard]] std::vector<float> resample(std::span<const float> input, double fromRate, double toRate);

} // namespace snotes::transcription
