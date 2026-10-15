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
/// DirectSound on Windows (F-AO-03); ALSA, then JACK on Linux (F-AO-02). Types the list does not know
/// follow in their given order. Types without an input device can only play, not record (F-AU-01), so
/// they follow all types with inputs. failedType, whose chosen device just failed, comes last: its other
/// devices are the last resort (another ASIO driver is rarely a good replacement).
[[nodiscard]] std::vector<std::string> driverTypesByPreference(const std::vector<DriverDevices>& drivers,
                                                               std::string_view failedType = {});

} // namespace snotes::audio
