#pragma once

#include <string>

namespace snotes::ui
{

/// Length of a recording for the top bar, e.g. "1:05,3" (minutes, seconds, tenths).
[[nodiscard]] std::string formatRecordingTime(double seconds);

} // namespace snotes::ui
