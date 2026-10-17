#include "ui/LevelMeter.h"

#include <algorithm>
#include <cmath>

namespace snotes::ui
{

void LevelMeter::update(float peak, double seconds)
{
    constexpr float kDecibelsPerDecade = 20.0F;
    const float peakDecibels = peak > 0.0F ? kDecibelsPerDecade * std::log10(peak) : kFloorDecibels;
    const auto fallen = decibels_ - kFallDecibelsPerSecond * static_cast<float>(seconds);
    decibels_ = std::max({peakDecibels, fallen, kFloorDecibels});
    if (peak >= kClipPeak)
        clipped_ = true;
}

float LevelMeter::position() const
{
    return std::clamp((decibels_ - kFloorDecibels) / -kFloorDecibels, 0.0F, 1.0F);
}

} // namespace snotes::ui
