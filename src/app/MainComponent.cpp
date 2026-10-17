#include "app/MainComponent.h"

#include "i18n/Translate.h"
#include "model/ModelIds.h"
#include "model/Project.h"
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
constexpr int kSmallButtonWidth = 36;
constexpr int kStatusHeight = 24;
constexpr int kStatusIntervalMs = 50;
constexpr int kRecordButtonWidth = 90;
constexpr int kTimeWidth = 90;
constexpr int kClickButtonWidth = 70;
constexpr int kTempoWidth = 110;

} // namespace

MainComponent::MainComponent(juce::AudioDeviceManager& deviceManager,
                             const AudioDeviceSupervisor& deviceSupervisor,
                             TrackViewContext tracks,
                             std::function<void()> addVoice)
    : deviceManager_(deviceManager), deviceSupervisor_(deviceSupervisor), recording_(tracks.recording),
      transport_(tracks.transport), editor_(tracks.editor), project_(tracks.project),
      tracks_(std::move(tracks))
{
    addVoiceButton_.setButtonText("+");
    addVoiceButton_.setTooltip(i18n::trJuce("Add a voice on the next free input"));
    addVoiceButton_.onClick = std::move(addVoice);
    setUpTransport();
    setUpClick();
    for (auto* component : std::initializer_list<juce::Component*>{&addVoiceButton_,
                                                                   &startButton_,
                                                                   &playButton_,
                                                                   &recordButton_,
                                                                   &position_,
                                                                   &clickButton_,
                                                                   &clickTempo_,
                                                                   &tracks_,
                                                                   &status_})
        addAndMakeVisible(component);

    setSize(kWidth, kHeight);
    deviceManager_.addChangeListener(this);
    timerCallback();
    startTimer(kStatusIntervalMs);
}

void MainComponent::setUpTransport()
{
    startButton_.setButtonText(juce::String::fromUTF8("|◀"));
    startButton_.setTooltip(i18n::trJuce("Back to the start (Home)"));
    startButton_.onClick = [this] { transport_.returnToStart(); };
    playButton_.setTooltip(i18n::trJuce("Play or stop (space bar)"));
    playButton_.setColour(juce::TextButton::buttonOnColourId, theme::colours().accent);
    playButton_.onClick = [this]
    {
        transport_.togglePlay();
        showTransportState();
    };
    recordButton_.setButtonText(juce::String::fromUTF8("● Rec"));
    recordButton_.setTooltip(i18n::trJuce("Record all armed voices while the others play (Ctrl+R)"));
    recordButton_.setColour(juce::TextButton::buttonOnColourId, theme::colours().record);
    recordButton_.onClick = [this]
    {
        recording_.toggle();
        showTransportState();
    };
    position_.setFont(
        juce::FontOptions{juce::Font::getDefaultMonospacedFontName(), 15.0F, juce::Font::plain});
    position_.setTooltip(i18n::trJuce("Song position"));
}

void MainComponent::setUpClick()
{
    clickButton_.setButtonText(i18n::trJuce("Click"));
    clickButton_.setTooltip(i18n::trJuce("Play a click in the tempo on the right (Ctrl+K)"));
    clickButton_.setClickingTogglesState(true);
    clickButton_.onClick = [this] { editor_.setClickOn(clickButton_.getToggleState()); };
    clickTempo_.setSliderStyle(juce::Slider::IncDecButtons);
    clickTempo_.setRange(model::Mix::kMinClickBpm, model::Mix::kMaxClickBpm, 1.0);
    clickTempo_.setTextValueSuffix(" bpm");
    clickTempo_.setTooltip(i18n::trJuce("Tempo of the click in beats per minute"));
    clickTempo_.onValueChange = [this] { editor_.setClickBpm(clickTempo_.getValue()); };
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
    const auto place = [&bar](juce::Component& component, int width)
    {
        component.setBounds(bar.removeFromLeft(width).reduced(0, kPadding / 2));
        bar.removeFromLeft(kPadding);
    };
    place(addVoiceButton_, kSmallButtonWidth);
    bar.removeFromLeft(kPadding);
    place(startButton_, kSmallButtonWidth);
    place(playButton_, kSmallButtonWidth);
    place(recordButton_, kRecordButtonWidth);
    place(position_, kTimeWidth);
    clickTempo_.setBounds(bar.removeFromRight(kTempoWidth).reduced(0, kPadding / 2));
    bar.removeFromRight(kPadding / 2);
    clickButton_.setBounds(bar.removeFromRight(kClickButtonWidth).reduced(0, kPadding / 2));
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
    showTransportState();
    showClick();
    status_.setText(juce::String::fromUTF8(
                        ui::audioStatusText(describeDevice(deviceManager_, deviceSupervisor_)).c_str()),
                    juce::dontSendNotification);
}

void MainComponent::showTransportState()
{
    const bool recording = recording_.state() != ui::RecordingController::State::idle;
    const bool playing = transport_.isPlaying();
    recordButton_.setToggleState(recording, juce::dontSendNotification);
    playButton_.setToggleState(playing, juce::dontSendNotification);
    playButton_.setButtonText(juce::String::fromUTF8(playing ? "■" : "▶"));
    position_.setText(juce::String::fromUTF8(ui::formatRecordingTime(transport_.positionSeconds()).c_str()),
                      juce::dontSendNotification);
}

void MainComponent::showClick()
{
    const model::Mix mix{project_.getChildWithName(model::ids::mix), nullptr};
    clickButton_.setToggleState(mix.isClickOn(), juce::dontSendNotification);
    if (!clickTempo_.isMouseButtonDown())
        clickTempo_.setValue(mix.clickBpm(), juce::dontSendNotification);
}

} // namespace snotes::app
