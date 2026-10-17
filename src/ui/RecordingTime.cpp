#include "ui/RecordingTime.h"

#include "i18n/Translate.h"

namespace snotes::ui
{

std::string formatRecordingTime(double seconds)
{
    constexpr int kTenthsPerSecond = 10;
    constexpr int kTenthsPerMinute = 600;
    const auto tenths = static_cast<int>(seconds * kTenthsPerSecond);
    const int wholeSeconds = (tenths % kTenthsPerMinute) / kTenthsPerSecond;
    return std::to_string(tenths / kTenthsPerMinute) + ":" + (wholeSeconds < kTenthsPerSecond ? "0" : "") +
           std::to_string(wholeSeconds) + i18n::decimalSeparator() +
           std::to_string(tenths % kTenthsPerSecond);
}

} // namespace snotes::ui
