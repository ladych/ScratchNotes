#pragma once

#include "i18n/LanguageCatalog.h"
#include "ui/ThemeSetting.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <string>
#include <vector>

namespace snotes::app
{

/// Actions of the menu "Audio".
struct AudioMenuActions
{
    std::function<void()> showSettings;
    std::function<void()> toggleTestTone;
    std::function<bool()> testToneOn;
};

/// Actions of the menu "Hilfe", with the submenus "Sprache" and "Farbschema".
struct HelpActions
{
    std::function<void()> showAbout;
    std::function<std::vector<i18n::LanguageChoice>()> languages;
    std::function<void(const std::string& setting)> chooseLanguage;
    std::function<std::vector<ui::ThemeChoice>()> themes;
    std::function<void(const std::string& setting)> chooseTheme;
};

/// Humble object (E-03): menus "Datei", "Audio" and "Hilfe" with their shortcuts, forwarding every command
/// to the actions.
class MainMenu final : public juce::MenuBarModel, public juce::ApplicationCommandTarget
{
public:
    MainMenu(std::function<void()> quit, AudioMenuActions audio, HelpActions help);
    ~MainMenu() override;

    MainMenu(const MainMenu&) = delete;
    MainMenu& operator=(const MainMenu&) = delete;
    MainMenu(MainMenu&&) = delete;
    MainMenu& operator=(MainMenu&&) = delete;

    /// Key listener for the main window.
    [[nodiscard]] juce::KeyListener* keyMappings() { return commands_.getKeyMappings(); }

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    /// Entries of the submenus "Sprache" and "Farbschema"; the commands go through perform().
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    juce::ApplicationCommandTarget* getNextCommandTarget() override { return nullptr; }
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

private:
    [[nodiscard]] std::function<void()> actionFor(juce::CommandID commandID) const;

    std::function<void()> quit_;
    AudioMenuActions audio_;
    HelpActions help_;
    juce::ApplicationCommandManager commands_;
};

} // namespace snotes::app
