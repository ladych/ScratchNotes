#include "app/AppComposition.h"

#include "LanguageData.h"

#include "app/Dialogs.h"
#include "app/MainComponent.h"
#include "io/Utf8Path.h"
#include "model/ProjectFactory.h"
#include "ui/AboutInfo.h"
#include "ui/VoiceList.h"

#include <filesystem>
#include <span>
#include <utility>

namespace snotes::app
{
namespace
{

constexpr double kTestToneFrequencyHz = 440.0;
constexpr float kTestToneGain = 0.2F;
/// Opened on a device without saved settings; JUCE opens only the channels the device has. 32 covers the
/// interfaces of the project (UR44C, Clarett 2Pre, SSL 12) with every input.
constexpr AudioDeviceSupervisor::ChannelCounts kDefaultChannels{.inputs = 32, .outputs = 2};
#if defined(JUCE_ASIO) && JUCE_ASIO
constexpr bool kWithAsio = true;
#else
constexpr bool kWithAsio = false;
#endif

std::filesystem::path settingsFile(const char* name)
{
    const auto file = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                          .getChildFile("ScratchNotes")
                          .getChildFile(name);
    const std::string utf8 = file.getFullPathName().toStdString();
    // JUCE strings are UTF-8; std::filesystem only treats char8_t input as UTF-8 on every platform.
    return {std::u8string{utf8.begin(), utf8.end()}};
}

/// New projects get their folder here (F-PJ-01).
std::filesystem::path projectsFolder()
{
    return io::pathFromUtf8(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                                .getChildFile("Scratch Notes")
                                .getFullPathName()
                                .toStdString());
}

/// The language of the operating system, e.g. "de-DE".
std::string systemLanguage()
{
    return juce::SystemStats::getDisplayLanguage().toStdString();
}

/// The translation files built into the program (resources/Languages), with the saved language applied.
i18n::LanguageCatalog appliedLanguages(const io::SettingsStore& settings)
{
    i18n::LanguageCatalog catalog;
    for (const auto* name : std::span{&LanguageData::namedResourceList[0],
                                      static_cast<std::size_t>(LanguageData::namedResourceListSize)})
    {
        int size = 0;
        const auto* data = LanguageData::getNamedResource(name, size);
        const juce::String fileName{LanguageData::getNamedResourceOriginalFilename(name)};
        catalog.add(fileName.upToLastOccurrenceOf(".", false, false).toStdString(),
                    std::string{data, static_cast<std::size_t>(size)});
    }
    catalog.apply(
        catalog.resolve(settings.load().value_or(i18n::LanguageCatalog::kSystemLanguage), systemLanguage()));
    return catalog;
}

} // namespace

AppComposition::AppComposition(theme::LookAndFeel& lookAndFeel, std::function<void()> quit)
    : quit_(std::move(quit)), languageSettings_(fileSystem_, settingsFile("language.txt")),
      languages_(appliedLanguages(languageSettings_)), themeSettings_(fileSystem_, settingsFile("theme.txt")),
      themeSetting_(
          themeSettings_,
          [] { return juce::Desktop::getInstance().isDarkModeActive(); },
          [&lookAndFeel](ui::Theme shown)
          {
              lookAndFeel.setTheme(shown);
              theme::refreshWindows();
          }),
      project_(model::ProjectFactory::createDefault({})), snapshotPublisher_(project_, snapshots_),
      projectRepository_(fileSystem_), lastProjectSettings_(fileSystem_, settingsFile("last-project.txt")),
      document_(
          project_,
          undoManager_,
          projectRepository_,
          documentView_,
          {.projectsFolder = projectsFolder(),
           .timestamp = [] { return juce::Time::getCurrentTime().formatted("%Y-%m-%d %H-%M").toStdString(); },
           .rememberFolder = [this](const std::filesystem::path& folder)
           { lastProjectSettings_.save(io::utf8FromPath(folder)); }}),
      deviceSettings_(fileSystem_, settingsFile("audio-device.xml")),
      testTone_(kTestToneFrequencyHz, kTestToneGain), audioCallback_(testTone_),
      deviceSupervisor_(deviceManager_, deviceSettings_, audioCallback_, kDefaultChannels),
      mainMenu_(
          [this] { requestQuit(); },
          {.newProject = [this] { document_.newProject(); },
           .open = [this] { document_.open(); },
           .save = [this] { document_.save(); },
           .saveAs = [this] { document_.saveAs(); }},
          {.undo = [this] { document_.undo(); },
           .redo = [this] { document_.redo(); },
           .canUndo = [this] { return document_.canUndo(); },
           .canRedo = [this] { return document_.canRedo(); },
           .addVoice = [this] { addVoice(); }},
          {.showSettings = [this] { showAudioSettings(); },
           .toggleTestTone = [this] { testTone_.setEnabled(!testTone_.isEnabled()); },
           .testToneOn = [this] { return testTone_.isEnabled(); }},
          {.showAbout = [this]
           { showAboutDialog(ui::makeAboutInfo(JUCE_APPLICATION_VERSION_STRING, kWithAsio), dialogParent_); },
           .languages =
               [this]
           {
               return languages_.choices(
                   languageSettings_.load().value_or(i18n::LanguageCatalog::kSystemLanguage),
                   systemLanguage());
           },
           .chooseLanguage = [this](const std::string& setting) { chooseLanguage(setting); },
           .themes = [this] { return themeSetting_.choices(); },
           .chooseTheme = [this](const std::string& setting) { themeSetting_.choose(setting); }})
{
    const auto lastProject = lastProjectSettings_.load();
    document_.start(lastProject ? std::optional{io::pathFromUtf8(*lastProject)} : std::nullopt);
    themeSetting_.apply();
    juce::Desktop::getInstance().addDarkModeSettingListener(this);
    deviceSupervisor_.open();
    deviceManager_.addAudioCallback(&audioCallback_);
    deviceManager_.addChangeListener(this);
}

AppComposition::~AppComposition()
{
    juce::Desktop::getInstance().removeDarkModeSettingListener(this);
    deviceManager_.removeChangeListener(this);
    deviceManager_.removeAudioCallback(&audioCallback_);
    deviceManager_.closeAudioDevice();
}

std::unique_ptr<juce::Component> AppComposition::createMainComponent()
{
    return std::make_unique<MainComponent>(
        deviceManager_,
        deviceSupervisor_,
        testTone_,
        project_,
        [this] { showAudioSettings(); },
        [this] { addVoice(); });
}

void AppComposition::requestQuit()
{
    document_.requestClose(quit_);
}

void AppComposition::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    if (const auto xml = deviceManager_.createStateXml())
        deviceSettings_.save(xml->toString().toStdString());
}

void AppComposition::chooseLanguage(const std::string& setting)
{
    // An empty setting follows the system language, like no setting at all.
    languageSettings_.save(setting);
    const auto code = languages_.resolve(setting, systemLanguage());
    juce::AlertWindow::showMessageBoxAsync(
        juce::MessageBoxIconType::InfoIcon,
        juce::String::fromUTF8(languages_.translate(code, "Language").c_str()),
        juce::String::fromUTF8(languages_.restartNotice(setting, systemLanguage()).c_str()));
}

void AppComposition::addVoice()
{
    auto* device = deviceManager_.getCurrentAudioDevice();
    const int inputs = device != nullptr ? device->getInputChannelNames().size() : 0;
    model::Project project{project_, &undoManager_};
    static_cast<void>(ui::addVoice(project, idGenerator_, inputs));
}

void AppComposition::showAudioSettings()
{
    showAudioSettingsDialog(deviceManager_, dialogParent_);
}

} // namespace snotes::app
