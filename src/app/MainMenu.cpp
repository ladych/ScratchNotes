#include "app/MainMenu.h"

#include "i18n/Translate.h"

#include <array>
#include <utility>
#include <vector>

namespace snotes::app
{
namespace
{

namespace command
{
constexpr juce::CommandID fileQuit = 0x1001;
constexpr juce::CommandID audioSettings = 0x2001;
constexpr juce::CommandID audioTestTone = 0x2002;
constexpr juce::CommandID helpAbout = 0x9001;
} // namespace command

constexpr int kCtrl = juce::ModifierKeys::commandModifier;

struct CommandSpec
{
    juce::CommandID id;
    const char* name; ///< English, translated when shown
    const char* description;
    const char* category;
    char key; ///< 0: no shortcut
    int modifiers;
};

constexpr std::array kCommands{
    CommandSpec{command::fileQuit,
                i18n::marked("Quit"),
                i18n::marked("Quit the program"),
                i18n::marked("File"),
                'q',
                kCtrl},
    CommandSpec{command::audioSettings,
                i18n::marked("Audio settings..."),
                i18n::marked("Choose driver, device, inputs, outputs and buffer size"),
                i18n::marked("Audio"),
                ',',
                kCtrl},
    CommandSpec{command::audioTestTone,
                i18n::marked("Test tone"),
                i18n::marked("Play a sine tone on all outputs"),
                i18n::marked("Audio"),
                't',
                kCtrl},
    CommandSpec{command::helpAbout,
                i18n::marked("About and licenses..."),
                i18n::marked("Version, license and the components of other authors"),
                i18n::marked("Help"),
                0,
                0},
};

constexpr int kHelpMenu = 2;
/// Item IDs of the submenus "Sprache" and "Farbschema", outside the command IDs.
constexpr int kFirstLanguageItem = 0x9100;
constexpr int kFirstThemeItem = 0x9200;

/// Entries of the menus in the order of getMenuBarNames().
std::vector<juce::CommandID> menuCommands(int menuIndex)
{
    switch (menuIndex)
    {
    case 0:
        return {command::fileQuit};
    case 1:
        return {command::audioSettings, command::audioTestTone};
    default:
        return {command::helpAbout};
    }
}

const CommandSpec* findCommand(juce::CommandID id)
{
    for (const auto& spec : kCommands)
        if (spec.id == id)
            return &spec;
    return nullptr;
}

} // namespace

MainMenu::MainMenu(std::function<void()> quit, AudioMenuActions audio, HelpActions help)
    : quit_(std::move(quit)), audio_(std::move(audio)), help_(std::move(help))
{
    commands_.registerAllCommandsForTarget(this);
    commands_.setFirstCommandTarget(this);
    setApplicationCommandManagerToWatch(&commands_);
}

MainMenu::~MainMenu()
{
    setApplicationCommandManagerToWatch(nullptr);
}

juce::StringArray MainMenu::getMenuBarNames()
{
    juce::StringArray names;
    for (const auto* name : {i18n::marked("File"), i18n::marked("Audio"), i18n::marked("Help")})
        names.add(i18n::trJuce(name));
    return names;
}

juce::PopupMenu MainMenu::getMenuForIndex(int menuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;
    for (const auto id : menuCommands(menuIndex))
        menu.addCommandItem(&commands_, id);
    if (menuIndex == kHelpMenu)
    {
        juce::PopupMenu languages;
        int itemId = kFirstLanguageItem;
        for (const auto& choice : help_.languages())
            languages.addItem(itemId++, juce::String::fromUTF8(choice.label.c_str()), true, choice.ticked);
        juce::PopupMenu themes;
        itemId = kFirstThemeItem;
        for (const auto& choice : help_.themes())
            themes.addItem(itemId++, juce::String::fromUTF8(choice.label.c_str()), true, choice.ticked);
        menu.addSeparator();
        menu.addSubMenu(i18n::trJuce("Language"), languages);
        menu.addSubMenu(i18n::trJuce("Colour scheme"), themes);
    }
    return menu;
}

void MainMenu::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
    if (topLevelMenuIndex != kHelpMenu || menuItemID < kFirstLanguageItem)
        return;
    if (menuItemID >= kFirstThemeItem)
    {
        const auto choices = help_.themes();
        const auto index = static_cast<std::size_t>(menuItemID - kFirstThemeItem);
        if (index < choices.size())
            help_.chooseTheme(choices[index].setting);
        return;
    }
    const auto choices = help_.languages();
    const auto index = static_cast<std::size_t>(menuItemID - kFirstLanguageItem);
    if (index < choices.size())
        help_.chooseLanguage(choices[index].setting);
}

void MainMenu::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    for (const auto& spec : kCommands)
        commands.add(spec.id);
}

void MainMenu::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    const auto* spec = findCommand(commandID);
    if (spec == nullptr)
        return;
    result.setInfo(
        i18n::trJuce(spec->name), i18n::trJuce(spec->description), i18n::trJuce(spec->category), 0);
    if (spec->key != 0)
        result.addDefaultKeypress(spec->key, spec->modifiers);
    if (commandID == command::audioTestTone)
        result.setTicked(audio_.testToneOn());
}

bool MainMenu::perform(const InvocationInfo& info)
{
    const auto action = actionFor(info.commandID);
    if (!action)
        return false;
    action();
    return true;
}

std::function<void()> MainMenu::actionFor(juce::CommandID commandID) const
{
    switch (commandID)
    {
    case command::fileQuit:
        return quit_;
    case command::audioSettings:
        return audio_.showSettings;
    case command::audioTestTone:
        return audio_.toggleTestTone;
    case command::helpAbout:
        return help_.showAbout;
    default:
        return {};
    }
}

} // namespace snotes::app
