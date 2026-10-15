#include "audio/DriverChoice.h"

#include <algorithm>
#include <array>
#include <iterator>

namespace snotes::audio
{
namespace
{

// The type names JUCE gives the drivers.
constexpr std::array<std::string_view, 7> kPreferredTypes{"ASIO",
                                                          "Windows Audio",
                                                          "Windows Audio (Low Latency Mode)",
                                                          "Windows Audio (Exclusive Mode)",
                                                          "DirectSound",
                                                          "ALSA",
                                                          "JACK"};

std::size_t rank(const std::string& type)
{
    return static_cast<std::size_t>(
        std::distance(kPreferredTypes.begin(), std::ranges::find(kPreferredTypes, type)));
}

constexpr std::size_t kWithoutInputs = kPreferredTypes.size() + 1;
constexpr std::size_t kLastRank = 2 * kWithoutInputs;

} // namespace

std::vector<std::string> driverTypesByPreference(const std::vector<DriverDevices>& drivers,
                                                 std::string_view failedType)
{
    std::vector<const DriverDevices*> playable;
    for (const auto& driver : drivers)
        if (!driver.outputs.empty())
            playable.push_back(&driver);
    std::ranges::stable_sort(playable,
                             {},
                             [failedType](const DriverDevices* driver)
                             {
                                 if (driver->type == failedType)
                                     return kLastRank;
                                 return rank(driver->type) + (driver->inputs.empty() ? kWithoutInputs : 0);
                             });
    std::vector<std::string> types;
    types.reserve(playable.size());
    for (const auto* driver : playable)
        types.push_back(driver->type);
    return types;
}

} // namespace snotes::audio
