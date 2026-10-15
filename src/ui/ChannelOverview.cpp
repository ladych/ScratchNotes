#include "ui/ChannelOverview.h"

#include "i18n/Translate.h"

#include <cstddef>

namespace snotes::ui
{

std::vector<ChannelEntry> channelEntries(const std::vector<std::string>& names,
                                         const std::vector<bool>& active)
{
    std::vector<ChannelEntry> entries;
    entries.reserve(names.size());
    for (std::size_t index = 0; index < names.size(); ++index)
    {
        const bool on = index < active.size() && active[index];
        auto text = std::to_string(index + 1);
        if (!names[index].empty())
            text += "  " + names[index];
        if (!on)
            text += i18n::tr(" (off)");
        entries.push_back({.text = text, .active = on});
    }
    return entries;
}

} // namespace snotes::ui
