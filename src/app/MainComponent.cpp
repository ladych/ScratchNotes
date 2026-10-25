#include "app/MainComponent.h"

#include "i18n/Translate.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "ui/AudioStatus.h"
#include "ui/RecordingTime.h"
#include "ui/RhythmUpdate.h"
#include "ui/ScoreDisplay.h"

#include <initializer_list>
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
constexpr int kViewButtonWidth = 64;
constexpr int kViewGroup = 4711;
constexpr int kMeterWidth = 110;

} // namespace

MainComponent::MainComponent(juce::AudioDeviceManager& deviceManager,
                             const AudioDeviceSupervisor& deviceSupervisor,
                             TrackViewContext tracks,
                             std::function<void()> addVoice)
    : deviceManager_(deviceManager), deviceSupervisor_(deviceSupervisor), recording_(tracks.recording),
      transcription_(tracks.transcription), transport_(tracks.transport), editor_(tracks.editor),
      grid_(tracks.grid), tapper_(tracks.tapper), project_(tracks.project), tracks_(std::move(tracks)),
      score_(tracks_.context().project, tracks_.context().projectFolder)
{
    addVoiceButton_.setButtonText("+");
    addVoiceButton_.setTooltip(i18n::trJuce("Add a voice on the next free input"));
    addVoiceButton_.onClick = std::move(addVoice);
    setUpTransport();
    setUpClick();
    setUpViews();
    setUpMeter();
    for (auto* component : std::initializer_list<juce::Component*>{&addVoiceButton_,
                                                                   &startButton_,
                                                                   &playButton_,
                                                                   &recordButton_,
                                                                   &position_,
                                                                   &meter_,
                                                                   &tracksButton_,
                                                                   &notesButton_,
                                                                   &bothButton_,
                                                                   &clickButton_,
                                                                   &clickTempo_,
                                                                   &tracks_,
                                                                   &score_,
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

void MainComponent::setUpMeter()
{
    meter_.setTooltip(i18n::trJuce("Meter: choose one of the readings of the beat analysis"));
    meter_.setTextWhenNothingSelected(i18n::trJuce("Meter"));
    meter_.setTextWhenNoChoicesAvailable(i18n::trJuce("No beat grid yet"));
    meter_.onChange = [this]
    {
        if (meter_.getSelectedItemIndex() >= 0)
            grid_.chooseMeter(static_cast<std::size_t>(meter_.getSelectedItemIndex()));
    };
}

void MainComponent::setUpViews()
{
    const auto setUp = [this](juce::TextButton& button, const juce::String& tip, model::Project::View view)
    {
        button.setTooltip(tip);
        button.setRadioGroupId(kViewGroup);
        button.setColour(juce::TextButton::buttonOnColourId, theme::colours().highlight);
        button.onClick = [this, view]
        {
            model::Project{project_, nullptr}.setView(view);
            resized();
            repaint();
        };
    };
    tracksButton_.setButtonText(i18n::trJuce("Tracks"));
    notesButton_.setButtonText(i18n::trJuce("Notes"));
    bothButton_.setButtonText(i18n::trJuce("Both"));
    setUp(tracksButton_, i18n::trJuce("Show the tracks with their waveforms"), model::Project::View::tracks);
    setUp(notesButton_, i18n::trJuce("Show the score with notes and tablature"), model::Project::View::notes);
    setUp(bothButton_, i18n::trJuce("Show tracks and score side by side"), model::Project::View::both);
    tracksButton_.setConnectedEdges(juce::Button::ConnectedOnRight);
    notesButton_.setConnectedEdges(juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
    bothButton_.setConnectedEdges(juce::Button::ConnectedOnLeft);
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
    if (tracks_.isVisible() && score_.isVisible())
    {
        g.setColour(theme::colours().gap);
        g.fillRect(
            tracks_.getRight(), tracks_.getY(), score_.getX() - tracks_.getRight(), tracks_.getHeight());
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    status_.setBounds(area.removeFromBottom(kStatusHeight).reduced(kPadding, 0));
    area.reduce(kPadding, kPadding);
    layOutBar(area.removeFromTop(kBarHeight));
    area.removeFromTop(kPadding);
    shownView_ = model::Project{project_, nullptr}.view();
    const auto widths = ui::viewWidths(shownView_, area.getWidth(), kPadding);
    tracks_.setVisible(widths.tracks > 0);
    score_.setVisible(widths.notes > 0);
    tracks_.setBounds(area.removeFromLeft(widths.tracks));
    score_.setBounds(area.removeFromRight(widths.notes));
    showView();
}

void MainComponent::layOutBar(juce::Rectangle<int> bar)
{
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
    place(meter_, kMeterWidth);
    clickTempo_.setBounds(bar.removeFromRight(kTempoWidth).reduced(0, kPadding / 2));
    bar.removeFromRight(kPadding / 2);
    clickButton_.setBounds(bar.removeFromRight(kClickButtonWidth).reduced(0, kPadding / 2));
    bar.removeFromRight(kPadding * 3);
    for (auto* button : {&bothButton_, &notesButton_, &tracksButton_})
        button->setBounds(bar.removeFromRight(kViewButtonWidth).reduced(0, kPadding / 2));
}

void MainComponent::showView()
{
    tracksButton_.setToggleState(shownView_ == model::Project::View::tracks, juce::dontSendNotification);
    notesButton_.setToggleState(shownView_ == model::Project::View::notes, juce::dontSendNotification);
    bothButton_.setToggleState(shownView_ == model::Project::View::both, juce::dontSendNotification);
}

void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    tracks_.refresh();
}

void MainComponent::timerCallback()
{
    recording_.poll();
    transcription_.poll();
    tapper_.poll();
    showTransportState();
    showClick();
    showMeters();
    // An opened project brings its own view.
    if (model::Project{project_, nullptr}.view() != shownView_)
    {
        resized();
        repaint();
    }
    // The note detection comes first, so it is seen next to a long audio status.
    auto status = tapper_.statusText();
    if (status.empty())
        status = transcription_.statusText();
    if (status.empty())
        status = ui::beatGridSummary(project_);
    if (!status.empty())
        status += "   ·   ";
    status += ui::audioStatusText(describeDevice(deviceManager_, deviceSupervisor_));
    status_.setText(juce::String::fromUTF8(status.c_str()), juce::dontSendNotification);
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

void MainComponent::showMeters()
{
    auto choices = grid_.meterChoices();
    if (choices == shownMeters_ || meter_.isPopupActive())
        return;
    shownMeters_ = std::move(choices);
    meter_.clear(juce::dontSendNotification);
    int id = 1;
    for (const auto& choice : shownMeters_)
    {
        meter_.addItem(juce::String::fromUTF8(choice.label.c_str()), id);
        if (choice.ticked)
            meter_.setSelectedId(id, juce::dontSendNotification);
        ++id;
    }
}

void MainComponent::showClick()
{
    const model::Mix mix{project_.getChildWithName(model::ids::mix), nullptr};
    clickButton_.setToggleState(mix.isClickOn(), juce::dontSendNotification);
    if (!clickTempo_.isMouseButtonDown())
        clickTempo_.setValue(mix.clickBpm(), juce::dontSendNotification);
}

} // namespace snotes::app
