#include "app/MainComponent.h"

#include "i18n/Translate.h"
#include "ui/AudioStatus.h"
#include "ui/RecordingTime.h"

#include <string>
#include <utility>

namespace snotes::app
{
namespace
{

constexpr int kWidth = 1200;
constexpr int kHeight = 720;
constexpr int kPadding = 8;
constexpr int kBarHeight = 40;
constexpr int kButtonWidth = 180;
constexpr int kStatusHeight = 24;
constexpr int kStatusIntervalMs = 100;
constexpr int kRecordButtonWidth = 90;
constexpr int kTimeWidth = 90;

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

} // namespace

MainComponent::MainComponent(juce::AudioDeviceManager& deviceManager,
                             const AudioDeviceSupervisor& deviceSupervisor,
                             TrackViewContext tracks,
                             std::function<void()> addVoice)
    : deviceManager_(deviceManager), deviceSupervisor_(deviceSupervisor), recording_(tracks.recording),
      tracks_(std::move(tracks))
{
    setUpTopBar(std::move(addVoice));
    for (auto* component : std::initializer_list<juce::Component*>{
             &recordButton_, &recordTime_, &addVoiceButton_, &tracks_, &status_})
        addAndMakeVisible(component);

    setSize(kWidth, kHeight);
    deviceManager_.addChangeListener(this);
    timerCallback();
    startTimer(kStatusIntervalMs);
}

void MainComponent::setUpTopBar(std::function<void()> addVoice)
{
    recordButton_.setButtonText(juce::String::fromUTF8("● Rec"));
    recordButton_.setTooltip(i18n::trJuce("Record all armed voices"));
    recordButton_.setColour(juce::TextButton::buttonOnColourId, theme::colours().record);
    recordButton_.onClick = [this]
    {
        recording_.toggle();
        showRecordingState();
    };
    recordTime_.setFont(
        juce::FontOptions{juce::Font::getDefaultMonospacedFontName(), 15.0F, juce::Font::plain});
    addVoiceButton_.setButtonText(i18n::trJuce("+ Voice"));
    addVoiceButton_.setTooltip(i18n::trJuce("Add a voice on the next free input"));
    addVoiceButton_.onClick = std::move(addVoice);
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
    recordButton_.setBounds(bar.removeFromLeft(kRecordButtonWidth).reduced(0, kPadding / 2));
    bar.removeFromLeft(kPadding);
    recordTime_.setBounds(bar.removeFromLeft(kTimeWidth));
    addVoiceButton_.setBounds(bar.removeFromRight(kButtonWidth).reduced(0, kPadding / 2));
    area.removeFromTop(kPadding);
    tracks_.setBounds(area);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    tracks_.refresh();
}

void MainComponent::timerCallback()
{
    recording_.poll();
    showRecordingState();
    status_.setText(
        juce::String::fromUTF8(ui::audioStatusText(deviceInfo(deviceManager_, deviceSupervisor_)).c_str()),
        juce::dontSendNotification);
}

void MainComponent::showRecordingState()
{
    const bool recording = recording_.state() != ui::RecordingController::State::idle;
    recordButton_.setToggleState(recording, juce::dontSendNotification);
    recordTime_.setText(juce::String::fromUTF8(ui::formatRecordingTime(recording_.recordedSeconds()).c_str()),
                        juce::dontSendNotification);
}

} // namespace snotes::app
