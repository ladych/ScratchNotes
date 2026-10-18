#include "app/Dialogs.h"

#include "app/AboutDialog.h"
#include "app/LatencyDialog.h"
#include "i18n/Translate.h"

#include <memory>

namespace snotes::app
{
namespace
{

/// Interfaces like the SSL 12 have more than 8 inputs; the dialog offers each channel on its own.
constexpr int kMaxChannels = 32;
constexpr int kSettingsWidth = 600;
constexpr int kSettingsHeight = 560;

void launch(std::unique_ptr<juce::Component> content,
            const juce::String& title,
            juce::Component* centreAround,
            bool resizable = false)
{
    juce::DialogWindow::LaunchOptions options;
    options.dialogBackgroundColour =
        content->getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    options.content.setOwned(content.release());
    options.dialogTitle = title;
    options.componentToCentreAround = centreAround;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = resizable;
    options.launchAsync();
}

} // namespace

void showAudioSettingsDialog(juce::AudioDeviceManager& deviceManager, juce::Component* centreAround)
{
    auto selector = std::make_unique<juce::AudioDeviceSelectorComponent>(deviceManager,
                                                                         0,
                                                                         kMaxChannels, // audio inputs
                                                                         0,
                                                                         kMaxChannels, // audio outputs
                                                                         false,        // MIDI inputs
                                                                         false,        // MIDI output
                                                                         false,        // stereo pairs
                                                                         false);
    selector->setSize(kSettingsWidth, kSettingsHeight);
    launch(std::move(selector), i18n::trJuce("Audio settings"), centreAround, true);
}

void showLatencyDialog(ui::LatencyCalibration& calibration,
                       ui::LatencyCompensation& compensation,
                       const std::vector<std::string>& inputNames,
                       juce::Component* centreAround)
{
    launch(std::make_unique<LatencyDialog>(calibration, compensation, inputNames),
           i18n::trJuce("Latency compensation"),
           centreAround);
}

void showAboutDialog(const ui::AboutInfo& info, juce::Component* centreAround)
{
    launch(std::make_unique<AboutDialog>(info), i18n::trJuce("About Scratch Notes"), centreAround, true);
}

} // namespace snotes::app
