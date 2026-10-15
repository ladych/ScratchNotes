#include "ui/AudioStatus.h"

#include "i18n/Translate.h"

#include <cmath>
#include <string>

namespace snotes::ui
{
namespace
{

constexpr double kMsPerSecond = 1000.0;
/// Upper limit of the round trip (Q-01); the target is 5 ms.
constexpr double kRoundTripLimitMs = 10.0;

/// One decimal with the decimal separator of the language.
std::string formatMs(double milliseconds)
{
    const long long tenths = std::llround(milliseconds * 10.0);
    return i18n::tr("%1 ms",
                    std::to_string(tenths / 10) + i18n::decimalSeparator() + std::to_string(tenths % 10));
}

double samplesToMs(int samples, const AudioDeviceInfo& info)
{
    return (samples > 0 ? samples : info.bufferSize) * kMsPerSecond / info.sampleRate;
}

std::string latencyText(const AudioDeviceInfo& info)
{
    const double roundTrip = roundTripLatencyMs(info);
    // Rounded first, so a value shown as "10,0 ms" is not marked as above 10 ms.
    const bool tooHigh = std::llround(roundTrip * 10.0) > std::llround(kRoundTripLimitMs * 10.0);
    return i18n::tr("Round trip %1%2 (input %3, output %4)",
                    formatMs(roundTrip),
                    tooHigh ? i18n::tr(" above 10 ms") : std::string{},
                    formatMs(inputLatencyMs(info)),
                    formatMs(outputLatencyMs(info)));
}

std::string channelText(const AudioDeviceInfo& info)
{
    const auto outputs = i18n::tr("%1 outputs", std::to_string(info.outputChannels));
    if (info.inputChannels <= 0)
        return i18n::tr("No input, recording not possible") + ", " + outputs;
    return i18n::tr("%1 inputs", std::to_string(info.inputChannels)) + ", " + outputs;
}

} // namespace

double inputLatencyMs(const AudioDeviceInfo& info)
{
    return samplesToMs(info.inputLatencySamples, info);
}

double outputLatencyMs(const AudioDeviceInfo& info)
{
    return samplesToMs(info.outputLatencySamples, info);
}

double roundTripLatencyMs(const AudioDeviceInfo& info)
{
    return inputLatencyMs(info) + outputLatencyMs(info);
}

std::string audioStatusText(const AudioDeviceInfo& info)
{
    std::string warning;
    if (info.stalled)
        warning = i18n::tr("Audio device not responding, retrying | ");
    if (!info.open || info.sampleRate <= 0.0)
        return warning + i18n::tr("No audio device open");
    std::string replaced;
    if (!info.replacedDevice.empty())
        replaced = i18n::tr("Replacing %1 | ", info.replacedDevice);
    std::string xruns;
    if (info.xruns >= 0)
        xruns = i18n::tr(" | XRuns: %1", std::to_string(info.xruns));
    const double bufferMs = info.bufferSize * kMsPerSecond / info.sampleRate;
    return warning + replaced + info.driver + " | " + info.device + " | " +
           i18n::tr("%1 Hz | Buffer %2 (%3) | ",
                    std::to_string(std::llround(info.sampleRate)),
                    std::to_string(info.bufferSize),
                    formatMs(bufferMs)) +
           latencyText(info) + " | " + channelText(info) + xruns;
}

} // namespace snotes::ui
