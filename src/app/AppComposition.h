#pragma once

#include "app/AudioCallback.h"
#include "app/AudioDeviceSupervisor.h"
#include "app/JuceDocumentView.h"
#include "app/MainMenu.h"
#include "app/Theme.h"
#include "engine/TestToneSource.h"
#include "i18n/LanguageCatalog.h"
#include "io/ProjectRepository.h"
#include "io/SettingsStore.h"
#include "io/StdFileSystem.h"
#include "model/SnapshotPublisher.h"
#include "model/UuidGenerator.h"
#include "ui/DocumentController.h"
#include "ui/ThemeSetting.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace snotes::app
{

/// Composition root (E-05): the only place where the object graph is created and wired. Members are
/// declared in dependency order, so they are destroyed in reverse.
class AppComposition final : private juce::ChangeListener, private juce::DarkModeSettingListener
{
public:
    /// The colour scheme is shown through `lookAndFeel`, the program's default look and feel.
    AppComposition(theme::LookAndFeel& lookAndFeel, std::function<void()> quit);
    ~AppComposition() override;

    AppComposition(const AppComposition&) = delete;
    AppComposition& operator=(const AppComposition&) = delete;
    AppComposition(AppComposition&&) = delete;
    AppComposition& operator=(AppComposition&&) = delete;

    [[nodiscard]] std::unique_ptr<juce::Component> createMainComponent();
    [[nodiscard]] MainMenu& mainMenu() { return mainMenu_; }
    /// Component the dialogs are centred on.
    void setDialogParent(juce::Component* parent) { dialogParent_ = parent; }
    /// Shows the window title with the project name.
    void setTitleSink(JuceDocumentView::TitleSink sink) { documentView_.setTitleSink(std::move(sink)); }
    /// Quits unless the user cancels the question for unsaved changes (F-PJ-02).
    void requestQuit();

private:
    /// Saves the device settings whenever they change (F-AO-04).
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    /// The operating system switched between light and dark mode.
    void darkModeSettingChanged() override { themeSetting_.systemChanged(); }
    /// Saves the language of "Hilfe > Sprache"; it shows after a restart.
    void chooseLanguage(const std::string& setting);
    void showAudioSettings();
    void addVoice();

    std::function<void()> quit_;
    io::StdFileSystem fileSystem_;
    // Language: applied before any other member, so every text is created in it.
    io::SettingsStore languageSettings_;
    i18n::LanguageCatalog languages_;
    // Colour scheme: applied before the components are created, which take its colours.
    io::SettingsStore themeSettings_;
    ui::ThemeSetting themeSetting_;
    // Project (AP1): one tree for the whole run; new and opened projects are copied into it.
    juce::UndoManager undoManager_;
    juce::ValueTree project_;
    model::ProjectSnapshotExchange snapshots_;
    model::SnapshotPublisher snapshotPublisher_;
    model::UuidGenerator idGenerator_;
    io::ProjectRepository projectRepository_;
    io::SettingsStore lastProjectSettings_;
    JuceDocumentView documentView_;
    ui::DocumentController document_;
    io::SettingsStore deviceSettings_;
    engine::TestToneSource testTone_;
    AudioCallback audioCallback_;
    juce::AudioDeviceManager deviceManager_;
    AudioDeviceSupervisor deviceSupervisor_;
    MainMenu mainMenu_;
    juce::Component* dialogParent_ = nullptr;
};

} // namespace snotes::app
