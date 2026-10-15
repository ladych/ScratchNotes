#include "app/MainComponent.h"

#include "i18n/Translate.h"
#include "ui/AudioStatus.h"
#include "ui/ChannelOverview.h"

#include <string>
#include <utility>
#include <vector>

namespace snotes::app
{
namespace
{

constexpr int kWidth = 900;
constexpr int kHeight = 560;
constexpr int kPadding = 8;
constexpr int kBarHeight = 40;
constexpr int kButtonWidth = 180;
constexpr int kStatusHeight = 24;
constexpr int kStatusIntervalMs = 500;

ui::AudioDeviceInfo deviceInfo(juce::AudioDeviceManager& deviceManager,
                               const AudioDeviceSupervisor& supervisor)
{
    ui::AudioDeviceInfo info;
    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        info.open = true;
        info.driver = device->getTypeName().toStdString();
        info.device = device->getName().toStdString();
        info.sampleRate = device->getCurrentSampleRate();
        info.bufferSize = device->getCurrentBufferSizeSamples();
        info.inputLatencySamples = device->getInputLatencyInSamples();
        info.outputLatencySamples = device->getOutputLatencyInSamples();
        info.inputChannels = device->getActiveInputChannels().countNumberOfSetBits();
        info.outputChannels = device->getActiveOutputChannels().countNumberOfSetBits();
    }
    supervisor.describe(info);
    return info;
}

std::vector<ui::ChannelEntry> entries(const juce::StringArray& names, const juce::BigInteger& active)
{
    std::vector<std::string> utf8Names;
    std::vector<bool> on;
    for (int index = 0; index < names.size(); ++index)
    {
        utf8Names.push_back(names[index].toStdString());
        on.push_back(active[index]);
    }
    return ui::channelEntries(utf8Names, on);
}

} // namespace

MainComponent::MainComponent(juce::AudioDeviceManager& deviceManager,
                             const AudioDeviceSupervisor& deviceSupervisor,
                             engine::TestToneSource& testTone,
                             std::function<void()> showAudioSettings)
    : deviceManager_(deviceManager), deviceSupervisor_(deviceSupervisor), testTone_(testTone),
      inputs_(i18n::trJuce("Inputs")), outputs_(i18n::trJuce("Outputs"))
{
    testToneButton_.setButtonText(i18n::trJuce("Test tone"));
    testToneButton_.setClickingTogglesState(true);
    testToneButton_.setToggleState(testTone_.isEnabled(), juce::dontSendNotification);
    testToneButton_.onClick = [this] { testTone_.setEnabled(testToneButton_.getToggleState()); };
    settingsButton_.setButtonText(i18n::trJuce("Audio settings..."));
    settingsButton_.onClick = std::move(showAudioSettings);
    for (auto* component : std::initializer_list<juce::Component*>{
             &testToneButton_, &settingsButton_, &inputs_, &outputs_, &status_})
        addAndMakeVisible(component);

    setSize(kWidth, kHeight);
    deviceManager_.addChangeListener(this);
    showChannels();
    timerCallback();
    startTimer(kStatusIntervalMs);
}

MainComponent::~MainComponent()
{
    stopTimer();
    deviceManager_.removeChangeListener(this);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(theme::colours().gap);
    g.setColour(theme::colours().panel);
    g.fillRect(getLocalBounds().withTrimmedBottom(kStatusHeight));
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    status_.setBounds(area.removeFromBottom(kStatusHeight).reduced(kPadding, 0));
    area.reduce(kPadding, kPadding);
    auto bar = area.removeFromTop(kBarHeight);
    testToneButton_.setBounds(bar.removeFromLeft(kButtonWidth).reduced(0, kPadding / 2));
    bar.removeFromLeft(kPadding);
    settingsButton_.setBounds(bar.removeFromLeft(kButtonWidth).reduced(0, kPadding / 2));
    area.removeFromTop(kPadding);
    inputs_.setBounds(area.removeFromLeft((area.getWidth() - kPadding) / 2));
    area.removeFromLeft(kPadding);
    outputs_.setBounds(area);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    showChannels();
}

void MainComponent::timerCallback()
{
    // The test tone can also be switched from the menu.
    testToneButton_.setToggleState(testTone_.isEnabled(), juce::dontSendNotification);
    status_.setText(
        juce::String::fromUTF8(ui::audioStatusText(deviceInfo(deviceManager_, deviceSupervisor_)).c_str()),
        juce::dontSendNotification);
}

void MainComponent::showChannels()
{
    auto* device = deviceManager_.getCurrentAudioDevice();
    if (device == nullptr)
    {
        inputs_.show({});
        outputs_.show({});
        return;
    }
    inputs_.show(entries(device->getInputChannelNames(), device->getActiveInputChannels()));
    outputs_.show(entries(device->getOutputChannelNames(), device->getActiveOutputChannels()));
}

} // namespace snotes::app
