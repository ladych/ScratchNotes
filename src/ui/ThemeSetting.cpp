#include "ui/ThemeSetting.h"

#include "i18n/Translate.h"

#include <juce_core/juce_core.h>

#include <utility>

namespace snotes::ui
{

ThemeSetting::ThemeSetting(io::SettingsStore& store,
                           std::function<bool()> systemIsDark,
                           std::function<void(Theme)> show)
    : store_(store), systemIsDark_(std::move(systemIsDark)), show_(std::move(show))
{
}

Theme ThemeSetting::theme() const
{
    const auto chosen = setting();
    if (chosen == kDark)
        return Theme::dark;
    if (chosen == kLight)
        return Theme::light;
    return systemTheme();
}

void ThemeSetting::apply() const
{
    show_(theme());
}

std::vector<ThemeChoice> ThemeSetting::choices() const
{
    const auto chosen = setting();
    const auto system = systemTheme() == Theme::dark ? i18n::tr("dark") : i18n::tr("light");
    std::vector<ThemeChoice> result;
    result.push_back({i18n::tr("System setting (%1)", system), kSystem, chosen == kSystem});
    result.push_back({i18n::tr("Dark"), kDark, chosen == kDark});
    result.push_back({i18n::tr("Light"), kLight, chosen == kLight});
    return result;
}

void ThemeSetting::choose(const std::string& setting)
{
    store_.save(setting);
    apply();
}

void ThemeSetting::systemChanged() const
{
    if (setting() == kSystem)
        apply();
}

std::string ThemeSetting::setting() const
{
    auto saved = juce::String{store_.load().value_or(kSystem)}.trim().toStdString();
    if (saved == kDark || saved == kLight)
        return saved;
    return kSystem;
}

Theme ThemeSetting::systemTheme() const
{
    return systemIsDark_() ? Theme::dark : Theme::light;
}

} // namespace snotes::ui
