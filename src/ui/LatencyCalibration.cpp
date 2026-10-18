#include "ui/LatencyCalibration.h"

#include "i18n/Translate.h"

#include <cmath>
#include <cstdlib>
#include <utility>
#include <vector>

namespace snotes::ui
{
namespace
{

constexpr double kMsPerSecond = 1000.0;

/// One decimal with the decimal separator of the language and a sign for negative values.
std::string formatMs(double milliseconds)
{
    const long long tenths = std::llround(milliseconds * 10.0);
    const std::string sign = tenths < 0 ? "-" : "";
    const long long magnitude = std::llabs(tenths);
    return i18n::tr("%1 ms",
                    sign + std::to_string(magnitude / 10) + i18n::decimalSeparator() +
                        std::to_string(magnitude % 10));
}

} // namespace

LatencyCalibration::LatencyCalibration(engine::LatencyProbe& probe,
                                       engine::Transport& transport,
                                       LatencyCompensation& compensation,
                                       std::function<AudioDeviceInfo()> device)
    : probe_(probe), transport_(transport), compensation_(compensation), device_(std::move(device))
{
}

void LatencyCalibration::start(int input)
{
    transport_.stop();
    probe_.start(input);
    waiting_ = true;
}

std::optional<std::string> LatencyCalibration::poll()
{
    if (!waiting_ || probe_.isRunning())
        return std::nullopt;
    waiting_ = false;
    if (probe_.numFinished() < engine::LatencyProbe::kPings)
        return std::nullopt;
    const auto info = device_();
    std::vector<std::int64_t> pings;
    pings.reserve(engine::LatencyProbe::kPings);
    for (int ping = 0; ping < engine::LatencyProbe::kPings; ++ping)
        pings.push_back(probe_.result(ping));
    const auto measured = measuredRoundTrip(pings, info.sampleRate);
    if (measured)
        static_cast<void>(compensation_.calibrate(*measured, info));
    return measurementText(measured, info);
}

std::string LatencyCalibration::compensationText() const
{
    const auto info = device_();
    if (!info.open)
        return i18n::tr("No audio device is open.");
    const double total = static_cast<double>(compensation_.samples(info)) * kMsPerSecond / info.sampleRate;
    return i18n::tr("New takes are moved by %1 (driver %2, offset %3).",
                    formatMs(total),
                    formatMs(roundTripLatencyMs(info)),
                    formatMs(compensation_.offsetMs()));
}

} // namespace snotes::ui
