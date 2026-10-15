#pragma once

#include <string>
#include <vector>

namespace snotes::ui
{

/// A line of the input or output list of the main window (F-AO-01).
struct ChannelEntry
{
    std::string text; ///< e.g. "3  Hi-Z 3", or "2  Mic/Line 2 (aus)" for a channel that is switched off
    bool active = false;
};

/// The device's channels numbered from 1, as the interface labels them. `active` says per channel whether it
/// is switched on in the audio settings; channels beyond it are off.
[[nodiscard]] std::vector<ChannelEntry> channelEntries(const std::vector<std::string>& names,
                                                       const std::vector<bool>& active);

} // namespace snotes::ui
