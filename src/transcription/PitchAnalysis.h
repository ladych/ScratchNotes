#pragma once

#include "transcription/BasicPitch.h"
#include "transcription/PitchModel.h"

#include <functional>
#include <optional>
#include <span>

namespace snotes::transcription
{

/// Called with the share of the work done (0..1); returns false to cancel (F-TX-01).
using ProgressCallback = std::function<bool(double done)>;

/// Port of run_inference (Basic Pitch 0.4.0): pads the 22.05 kHz audio, cuts it into overlapping windows,
/// runs the model on each and joins the frames without the overlaps (F-TX-02, E-12). Returns nothing if the
/// progress callback cancelled.
[[nodiscard]] std::optional<Activations>
analysePitches(std::span<const float> audio, IPitchModel& model, const ProgressCallback& progress);

} // namespace snotes::transcription
