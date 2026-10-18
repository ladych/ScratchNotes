#pragma once

#include <span>
#include <vector>

namespace snotes::rhythm
{

/// The rhythm analysis works on audio at 22.05 kHz, the rate of the pitch model.
inline constexpr double kAnalysisRate = 22050.0;
/// Hop of the onset strength the beat tracker follows (50 frames per second).
inline constexpr int kBeatHop = 441;
/// Hop of the finer onset strength for the meter features and the clean-up.
inline constexpr int kDetailHop = 256;

/// Spectral flux on a mel spectrogram (F-TX-04), as librosa.onset.onset_strength(y, sr=22050,
/// hop_length=hop): 2048-point frames with a Hann window, centred on hop * n, 128 mel bands in dB (80 dB
/// range), the mean rise over the bands from one frame to the next. One value per frame,
/// 1 + audio.size() / hop values.
[[nodiscard]] std::vector<float> onsetStrength(std::span<const float> audio, int hop);

/// Seconds of frame index of an envelope with the given hop.
[[nodiscard]] double frameSeconds(std::size_t frame, int hop);

} // namespace snotes::rhythm
