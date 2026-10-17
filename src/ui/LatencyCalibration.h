#pragma once

#include "engine/LatencyProbe.h"
#include "engine/Transport.h"
#include "ui/AudioStatus.h"
#include "ui/LatencyCompensation.h"

#include <functional>
#include <optional>
#include <string>

namespace snotes::ui
{

/// Presenter of the latency dialog (Q-01, Q-05, F-WI-05): runs the loopback measurement and makes its
/// result the compensation of new takes.
class LatencyCalibration
{
public:
    LatencyCalibration(engine::LatencyProbe& probe,
                       engine::Transport& transport,
                       LatencyCompensation& compensation,
                       std::function<AudioDeviceInfo()> device);

    /// Stops the playback, which would disturb the measurement, and pings through `input`.
    void start(int input);
    void cancel() { probe_.cancel(); }
    [[nodiscard]] bool isRunning() const { return probe_.isRunning(); }
    /// GUI timer: the result text once the measurement finished, nothing while it runs or after it was
    /// reported.
    [[nodiscard]] std::optional<std::string> poll();
    /// e.g. "Neue Takes werden um 8,0 ms verschoben (Treiber 8,0 ms, Versatz 0,0 ms)."
    [[nodiscard]] std::string compensationText() const;

private:
    engine::LatencyProbe& probe_;
    engine::Transport& transport_;
    LatencyCompensation& compensation_;
    std::function<AudioDeviceInfo()> device_;
    bool waiting_ = false;
};

} // namespace snotes::ui
