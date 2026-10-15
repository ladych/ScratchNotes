#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace snotes::audio
{

/// One audio driver type (e.g. "ASIO", "Windows Audio", "ALSA") with the input and output devices it offers.
struct DriverDevices
{
    std::string type;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
};

/// Driver types with at least one output device, best first: ASIO, then WASAPI as the fallback, then
/// DirectSound on Windows (F-AO-03); JACK, then ALSA on Linux (F-AO-02): a running JACK server (or PipeWire's
/// JACK) gives small buffers, ALSA's "default" device often only 512 samples and more. Types the list does
/// not know follow in their given order. Types without an input device can only play, not record (F-AU-01),
/// so they follow all types with inputs. failedType, whose chosen device just failed, comes last: its other
/// devices are the last resort (another ASIO driver is rarely a good replacement).
[[nodiscard]] std::vector<std::string> driverTypesByPreference(const std::vector<DriverDevices>& drivers,
                                                               std::string_view failedType = {});

} // namespace snotes::audio
