#pragma once

#include "io/SettingsStore.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// Colour scheme of the main window after the GUI draft v0.1 (dark and light).
enum class Theme : std::uint8_t
{
    dark,
    light
};

/// An entry of the menu "Help > Colour scheme".
struct ThemeChoice
{
    std::string label;   ///< in the current language
    std::string setting; ///< value to save, ThemeSetting::kSystem, kDark or kLight
    bool ticked = false; ///< the saved setting
};

/// The colour scheme as a program setting (AP0, F-UI-04, F-UI-05): the saved choice, or the light or dark
/// mode of the operating system if there is none. A change shows at once, without a restart.
class ThemeSetting
{
public:
    /// Setting value for "follow the operating system", the default.
    static constexpr const char* kSystem = "";
    static constexpr const char* kDark = "dark";
    static constexpr const char* kLight = "light";

    /// `systemIsDark` asks the operating system; `show` repaints the program in a theme.
    ThemeSetting(io::SettingsStore& store,
                 std::function<bool()> systemIsDark,
                 std::function<void(Theme)> show);

    /// The theme to show now.
    [[nodiscard]] Theme theme() const;
    /// Shows theme().
    void apply() const;

    /// "System setting (dark)", "Dark", "Light"; the saved setting is ticked.
    [[nodiscard]] std::vector<ThemeChoice> choices() const;
    /// Saves the setting and shows its theme.
    void choose(const std::string& setting);
    /// The operating system switched between light and dark mode: shown if the setting follows it.
    void systemChanged() const;

private:
    /// The saved setting, kSystem for none or an unknown one.
    [[nodiscard]] std::string setting() const;
    [[nodiscard]] Theme systemTheme() const;

    io::SettingsStore& store_;
    std::function<bool()> systemIsDark_;
    std::function<void(Theme)> show_;
};

} // namespace snotes::ui
