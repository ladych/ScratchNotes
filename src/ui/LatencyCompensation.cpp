#include "ui/LatencyCompensation.h"

#include "i18n/Translate.h"

#include <algorithm>
#include <charconv>
#include <cmath>

namespace snotes::ui
{
namespace
{

constexpr double kMsPerSecond = 1000.0;
constexpr double kMicrosecondsPerMs = 1000.0;
constexpr std::size_t kMinPings = 3;

double samplesToMs(std::int64_t samples, double sampleRate)
{
    return static_cast<double>(samples) * kMsPerSecond / sampleRate;
}

/// One decimal with the decimal separator of the language; for round trips, which are never negative.
std::string formatMs(double milliseconds)
{
    const long long tenths = std::llround(milliseconds * 10.0);
    return i18n::tr("%1 ms",
                    std::to_string(tenths / 10) + i18n::decimalSeparator() + std::to_string(tenths % 10));
}

} // namespace

LatencyCompensation::LatencyCompensation(io::SettingsStore& store) : store_(store)
{
    const auto saved = store_.load();
    if (!saved)
        return;
    long long microseconds = 0;
    const auto [end, error] = std::from_chars(saved->data(), saved->data() + saved->size(), microseconds);
    if (error == std::errc{})
        offsetMs_ = static_cast<double>(microseconds) / kMicrosecondsPerMs;
}

bool LatencyCompensation::setOffsetMs(double offsetMs)
{
    offsetMs_ = offsetMs;
    return store_.save(std::to_string(std::llround(offsetMs * kMicrosecondsPerMs)));
}

std::int64_t LatencyCompensation::samples(const AudioDeviceInfo& info) const
{
    const double milliseconds = roundTripLatencyMs(info) + offsetMs_;
    return std::max<std::int64_t>(std::llround(milliseconds * info.sampleRate / kMsPerSecond), 0);
}

bool LatencyCompensation::calibrate(std::int64_t measuredSamples, const AudioDeviceInfo& info)
{
    return setOffsetMs(samplesToMs(measuredSamples, info.sampleRate) - roundTripLatencyMs(info));
}

std::optional<std::int64_t> measuredRoundTrip(const std::vector<std::int64_t>& pings, double sampleRate)
{
    std::vector<std::int64_t> arrived;
    std::copy_if(
        pings.begin(), pings.end(), std::back_inserter(arrived), [](std::int64_t ping) { return ping >= 0; });
    if (arrived.size() < kMinPings)
        return std::nullopt;
    std::sort(arrived.begin(), arrived.end());
    if (samplesToMs(arrived.back() - arrived.front(), sampleRate) > 1.0)
        return std::nullopt;
    return arrived[arrived.size() / 2];
}

std::string measurementText(std::optional<std::int64_t> measuredSamples, const AudioDeviceInfo& info)
{
    if (!measuredSamples)
        return i18n::tr(
            "No clear signal arrived. Connect an output to the chosen input with a cable, turn the "
            "input up and try again.");
    const double measured = samplesToMs(*measuredSamples, info.sampleRate);
    return i18n::tr("Measured round trip %1, the driver reports %2. New takes are moved by %1.",
                    formatMs(measured),
                    formatMs(roundTripLatencyMs(info)));
}

} // namespace snotes::ui
