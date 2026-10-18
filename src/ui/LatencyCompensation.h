#pragma once

#include "io/SettingsStore.h"
#include "ui/AudioStatus.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// Latency compensation of new takes (F-WI-05): the round trip the driver reports plus an offset the user
/// sets or the built-in measurement finds (Q-01, Q-05). The offset is kept between runs.
class LatencyCompensation
{
public:
    explicit LatencyCompensation(io::SettingsStore& store);

    [[nodiscard]] double offsetMs() const { return offsetMs_; }
    /// Saves the offset; false if that failed (the offset applies anyway).
    bool setOffsetMs(double offsetMs);
    /// Round trip by which a new take is moved, in samples of the open device, at least 0.
    [[nodiscard]] std::int64_t samples(const AudioDeviceInfo& info) const;
    /// Sets the offset so the compensation equals a measured round trip.
    bool calibrate(std::int64_t measuredSamples, const AudioDeviceInfo& info);

private:
    io::SettingsStore& store_;
    double offsetMs_ = 0.0;
};

/// The measured round trip from the pings of the probe, ignoring pings that heard nothing; nothing unless
/// at least three pings arrived and agree within one millisecond.
[[nodiscard]] std::optional<std::int64_t> measuredRoundTrip(const std::vector<std::int64_t>& pings,
                                                            double sampleRate);
/// Result line of the measurement dialog.
[[nodiscard]] std::string measurementText(std::optional<std::int64_t> measuredSamples,
                                          const AudioDeviceInfo& info);

} // namespace snotes::ui
