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
constexpr juce::CommandID fileExportMidi = 0x1006;
constexpr juce::CommandID fileExportMusicXml = 0x1007;
constexpr juce::CommandID editUndo = 0x1101;
constexpr juce::CommandID editRedo = 0x1102;
constexpr juce::CommandID editAddVoice = 0x1103;
constexpr juce::CommandID recordToggle = 0x1201;
constexpr juce::CommandID transportPlay = 0x1202;
constexpr juce::CommandID transportStart = 0x1203;
constexpr juce::CommandID transportClick = 0x1204;
constexpr juce::CommandID audioSettings = 0x2001;
constexpr juce::CommandID audioTestTone = 0x2002;
constexpr juce::CommandID audioLatency = 0x2003;
constexpr juce::CommandID notesTranscribe = 0x3001;
constexpr juce::CommandID notesCancel = 0x3002;
constexpr juce::CommandID notesDetectGrid = 0x3003;
constexpr juce::CommandID notesTapBeat = 0x3004;
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
    CommandSpec{command::fileExportMidi,
                i18n::marked("Export as MIDI..."),
                i18n::marked("Save the score as MIDI file in sync with the recording"),
                i18n::marked("File"),
                0,
                0},
    CommandSpec{command::fileExportMusicXml,
                i18n::marked("Export as MusicXML..."),
                i18n::marked("Save the score with tablature and tempo as MusicXML file"),
                i18n::marked("File"),
                0,
                0},
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
    CommandSpec{command::transportPlay,
                i18n::marked("Play/Stop"),
                i18n::marked("Play the voices from the current position, or stop"),
                i18n::marked("Transport"),
                ' ',
                0},
    CommandSpec{command::transportStart,
                i18n::marked("Back to the start"),
                i18n::marked("Move the position to the start of the song"),
                i18n::marked("Transport"),
                0,
                0},
    CommandSpec{command::recordToggle,
                i18n::marked("Record"),
                i18n::marked("Start or stop recording all armed voices"),
                i18n::marked("Transport"),
                'r',
                kCtrl},
    CommandSpec{command::transportClick,
                i18n::marked("Click"),
                i18n::marked("Play a click in the tempo of the top bar"),
                i18n::marked("Transport"),
                'k',
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
    CommandSpec{
        command::audioLatency,
        i18n::marked("Latency compensation..."),
        i18n::marked("Measure the round trip with a loopback cable and set the compensation of new takes"),
        i18n::marked("Audio"),
        0,
        0},
    CommandSpec{command::notesTranscribe,
                i18n::marked("Detect notes"),
                i18n::marked("Detect the notes of the active take of every voice again"),
                i18n::marked("Notes"),
                't',
                kCtrlShift},
    CommandSpec{command::notesCancel,
                i18n::marked("Cancel note detection"),
                i18n::marked("Stop detecting notes; takes already done keep their notes"),
                i18n::marked("Notes"),
                0,
                0},
    CommandSpec{command::notesDetectGrid,
                i18n::marked("Detect beat grid again"),
                i18n::marked("Forget the corrections of meter and beats and detect them again"),
                i18n::marked("Notes"),
                0,
                0},
    CommandSpec{
        command::notesTapBeat,
        i18n::marked("Tap the beat"),
        i18n::marked("Tap every beat along during playback; the taps replace the beat grid when it stops"),
        i18n::marked("Notes"),
        'b',
        0},
    CommandSpec{command::helpAbout,
                i18n::marked("About and licenses..."),
                i18n::marked("Version, license and the components of other authors"),
                i18n::marked("Help"),
                0,
                0},
};

constexpr int kHelpMenu = 5;
/// Item IDs of the submenus "Sprache" and "Farbschema", outside the command IDs.
constexpr int kFirstLanguageItem = 0x9100;
constexpr int kFirstThemeItem = 0x9200;

/// Entries of the menus in the order of getMenuBarNames().
std::vector<juce::CommandID> menuCommands(int menuIndex)
{
    switch (menuIndex)
    {
    case 0:
        return {command::fileNew,
                command::fileOpen,
                command::fileSave,
                command::fileSaveAs,
                command::fileExportMidi,
                command::fileExportMusicXml,
                command::fileQuit};
    case 1:
        return {command::editUndo, command::editRedo, command::editAddVoice};
    case 2:
        return {
            command::transportPlay, command::transportStart, command::recordToggle, command::transportClick};
    case 3:
        return {command::audioSettings, command::audioLatency, command::audioTestTone};
    case 4:
        return {
            command::notesTranscribe, command::notesCancel, command::notesTapBeat, command::notesDetectGrid};
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
                   TransportActions transport,
                   AudioMenuActions audio,
                   NotesActions notes,
                   HelpActions help)
    : quit_(std::move(quit)), file_(std::move(file)), edit_(std::move(edit)),
      transport_(std::move(transport)), audio_(std::move(audio)), notes_(std::move(notes)),
      help_(std::move(help))
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
                             i18n::marked("Transport"),
                             i18n::marked("Audio"),
                             i18n::marked("Notes"),
                             i18n::marked("Help")})
        names.add(i18n::trJuce(name));
    return names;
}

juce::PopupMenu MainMenu::getMenuForIndex(int menuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;
    for (const auto id : menuCommands(menuIndex))
    {
        if (id == command::fileQuit || id == command::fileExportMidi || id == command::notesTapBeat ||
            id == command::editAddVoice || id == command::transportClick)
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
    if (commandID == command::transportStart)
        result.addDefaultKeypress(juce::KeyPress::homeKey, 0);
    if (commandID == command::transportPlay)
        result.setTicked(transport_.playing());
    if (commandID == command::recordToggle)
        result.setTicked(transport_.recording());
    if (commandID == command::transportClick)
        result.setTicked(transport_.clickOn());
    if (commandID == command::editUndo)
        result.setActive(edit_.canUndo());
    if (commandID == command::editRedo)
        result.setActive(edit_.canRedo());
    if (commandID == command::notesCancel)
        result.setActive(notes_.running());
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
    const std::array<std::pair<juce::CommandID, const std::function<void()>*>, 22> actions{{
        {command::fileQuit, &quit_},
        {command::fileNew, &file_.newProject},
        {command::fileOpen, &file_.open},
        {command::fileSave, &file_.save},
        {command::fileSaveAs, &file_.saveAs},
        {command::fileExportMidi, &file_.exportMidi},
        {command::fileExportMusicXml, &file_.exportMusicXml},
        {command::editUndo, &edit_.undo},
        {command::editRedo, &edit_.redo},
        {command::editAddVoice, &edit_.addVoice},
        {command::recordToggle, &transport_.toggleRecord},
        {command::transportPlay, &transport_.togglePlay},
        {command::transportStart, &transport_.returnToStart},
        {command::transportClick, &transport_.toggleClick},
        {command::audioLatency, &audio_.showLatency},
        {command::audioSettings, &audio_.showSettings},
        {command::audioTestTone, &audio_.toggleTestTone},
        {command::notesTranscribe, &notes_.transcribe},
        {command::notesCancel, &notes_.cancel},
        {command::notesDetectGrid, &notes_.detectGridAgain},
        {command::notesTapBeat, &notes_.tapBeat},
        {command::helpAbout, &help_.showAbout},
    }};
    for (const auto& [id, action] : actions)
        if (id == commandID)
            return *action;
    return {};
}

} // namespace snotes::app
