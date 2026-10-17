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
constexpr juce::CommandID fileNew = 0x1002;
constexpr juce::CommandID fileOpen = 0x1003;
constexpr juce::CommandID fileSave = 0x1004;
constexpr juce::CommandID fileSaveAs = 0x1005;
constexpr juce::CommandID editUndo = 0x1101;
constexpr juce::CommandID editRedo = 0x1102;
constexpr juce::CommandID editAddVoice = 0x1103;
constexpr juce::CommandID recordToggle = 0x1201;
constexpr juce::CommandID audioSettings = 0x2001;
constexpr juce::CommandID audioTestTone = 0x2002;
constexpr juce::CommandID helpAbout = 0x9001;
} // namespace command

constexpr int kCtrl = juce::ModifierKeys::commandModifier;
constexpr int kCtrlShift = juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier;

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
    CommandSpec{command::fileNew,
                i18n::marked("New project"),
                i18n::marked("Start a new project"),
                i18n::marked("File"),
                'n',
                kCtrl},
    CommandSpec{command::fileOpen,
                i18n::marked("Open project..."),
                i18n::marked("Open the projekt.snotes of a project folder"),
                i18n::marked("File"),
                'o',
                kCtrl},
    CommandSpec{command::fileSave,
                i18n::marked("Save"),
                i18n::marked("Save the project in its folder"),
                i18n::marked("File"),
                's',
                kCtrl},
    CommandSpec{command::fileSaveAs,
                i18n::marked("Save as..."),
                i18n::marked("Save the project with its audio in a new folder"),
                i18n::marked("File"),
                's',
                kCtrlShift},
    CommandSpec{command::editUndo,
                i18n::marked("Undo"),
                i18n::marked("Undo the last change"),
                i18n::marked("Edit"),
                'z',
                kCtrl},
    CommandSpec{command::editRedo,
                i18n::marked("Redo"),
                i18n::marked("Redo the last undone change"),
                i18n::marked("Edit"),
                'y',
                kCtrl},
    CommandSpec{command::editAddVoice,
                i18n::marked("Add voice"),
                i18n::marked("Add a voice on the next free input"),
                i18n::marked("Edit"),
                0,
                0},
    CommandSpec{command::recordToggle,
                i18n::marked("Record"),
                i18n::marked("Start or stop recording all armed voices"),
                i18n::marked("Recording"),
                'r',
                kCtrl},
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

constexpr int kHelpMenu = 4;
/// Item IDs of the submenus "Sprache" and "Farbschema", outside the command IDs.
constexpr int kFirstLanguageItem = 0x9100;
constexpr int kFirstThemeItem = 0x9200;

/// Entries of the menus in the order of getMenuBarNames().
std::vector<juce::CommandID> menuCommands(int menuIndex)
{
    switch (menuIndex)
    {
    case 0:
        return {
            command::fileNew, command::fileOpen, command::fileSave, command::fileSaveAs, command::fileQuit};
    case 1:
        return {command::editUndo, command::editRedo, command::editAddVoice};
    case 2:
        return {command::recordToggle};
    case 3:
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

MainMenu::MainMenu(std::function<void()> quit,
                   FileActions file,
                   EditActions edit,
                   RecordActions record,
                   AudioMenuActions audio,
                   HelpActions help)
    : quit_(std::move(quit)), file_(std::move(file)), edit_(std::move(edit)), record_(std::move(record)),
      audio_(std::move(audio)), help_(std::move(help))
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
    for (const auto* name : {i18n::marked("File"),
                             i18n::marked("Edit"),
                             i18n::marked("Recording"),
                             i18n::marked("Audio"),
                             i18n::marked("Help")})
        names.add(i18n::trJuce(name));
    return names;
}

juce::PopupMenu MainMenu::getMenuForIndex(int menuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;
    for (const auto id : menuCommands(menuIndex))
    {
        if (id == command::fileQuit || id == command::editAddVoice)
            menu.addSeparator();
        menu.addCommandItem(&commands_, id);
    }
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
    if (commandID == command::recordToggle)
        result.setTicked(record_.recording());
    if (commandID == command::editUndo)
        result.setActive(edit_.canUndo());
    if (commandID == command::editRedo)
        result.setActive(edit_.canRedo());
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
    case command::fileNew:
        return file_.newProject;
    case command::fileOpen:
        return file_.open;
    case command::fileSave:
        return file_.save;
    case command::fileSaveAs:
        return file_.saveAs;
    case command::editUndo:
        return edit_.undo;
    case command::editRedo:
        return edit_.redo;
    case command::editAddVoice:
        return edit_.addVoice;
    case command::recordToggle:
        return record_.toggle;
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
