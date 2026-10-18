#pragma once

#include "i18n/LanguageCatalog.h"
#include "ui/ThemeSetting.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <string>
#include <vector>

namespace snotes::app
{

/// Actions of the menu "Datei" besides "Beenden".
struct FileActions
{
    std::function<void()> newProject;
    std::function<void()> open;
    std::function<void()> save;
    std::function<void()> saveAs;
};

/// Actions of the menu "Bearbeiten".
struct EditActions
{
    std::function<void()> undo;
    std::function<void()> redo;
    std::function<bool()> canUndo;
    std::function<bool()> canRedo;
    std::function<void()> addVoice;
};

/// Actions of the menu "Transport" (F-WI-01, F-WI-07).
struct TransportActions
{
    std::function<void()> togglePlay;
    std::function<void()> returnToStart;
    std::function<void()> toggleRecord;
    std::function<void()> toggleClick;
    std::function<bool()> playing;
    std::function<bool()> recording;
    std::function<bool()> clickOn;
};

/// Actions of the menu "Audio".
struct AudioMenuActions
{
    std::function<void()> showSettings;
    std::function<void()> toggleTestTone;
    std::function<bool()> testToneOn;
    std::function<void()> showLatency;
};

/// Actions of the menu "Hilfe", with the submenus "Sprache" and "Farbschema".
struct NotesActions
{
    std::function<void()> transcribe;
    std::function<void()> cancel;
    std::function<bool()> running;
};

struct HelpActions
{
    std::function<void()> showAbout;
    std::function<std::vector<i18n::LanguageChoice>()> languages;
    std::function<void(const std::string& setting)> chooseLanguage;
    std::function<std::vector<ui::ThemeChoice>()> themes;
    std::function<void(const std::string& setting)> chooseTheme;
};

/// Humble object (E-03): menus "Datei", "Bearbeiten", "Transport", "Audio", "Noten" and "Hilfe" with their
/// shortcuts, forwarding every command to the actions.
class MainMenu final : public juce::MenuBarModel, public juce::ApplicationCommandTarget
{
public:
    MainMenu(std::function<void()> quit,
             FileActions file,
             EditActions edit,
             TransportActions transport,
             AudioMenuActions audio,
             NotesActions notes,
             HelpActions help);
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
    FileActions file_;
    EditActions edit_;
    TransportActions transport_;
    AudioMenuActions audio_;
    NotesActions notes_;
    HelpActions help_;
    juce::ApplicationCommandManager commands_;
};

} // namespace snotes::app
