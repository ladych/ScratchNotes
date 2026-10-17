#pragma once

namespace snotes::ui
{

/// Ballistics of one input meter (F-AU-03): rises at once, falls by kFallDecibelsPerSecond, and marks
/// clipping until the user clicks the meter (GUI-Entwurf v0.1, item 6).
class LevelMeter
{
public:
    static constexpr float kFloorDecibels = -60.0F;
    static constexpr float kFallDecibelsPerSecond = 24.0F;
    /// Interfaces clip at full scale; this leaves a little room for rounding in the driver.
    static constexpr float kClipPeak = 0.999F;

    /// GUI timer: `peak` is the highest absolute sample since the last update, `seconds` the time since.
    void update(float peak, double seconds);
    void resetClip() { clipped_ = false; }

    /// 0 at kFloorDecibels and below, 1 at full scale.
    [[nodiscard]] float position() const;
    [[nodiscard]] bool clipped() const { return clipped_; }

private:
    float decibels_ = kFloorDecibels;
    bool clipped_ = false;
};

} // namespace snotes::ui
