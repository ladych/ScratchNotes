#include "app/TrackView.h"

#include "app/Theme.h"
#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "model/Project.h"

#include <utility>

namespace snotes::app
{
namespace
{

constexpr int kRowHeight = 150;
constexpr int kHeaderWidth = 320;
constexpr int kMeterWidth = 7;
constexpr int kPadding = 6;
constexpr int kLineHeight = 26;
constexpr int kCaptionHeight = 14;
constexpr int kSmallButton = 30;
constexpr int kMeterIntervalMs = 33;
constexpr int kThumbnailResolution = 512;
constexpr int kThumbnailCacheSize = 32;
constexpr float kMillisecondsPerSecond = 1000.0F;

juce::String fromUtf8(const std::string& text)
{
    return juce::String::fromUTF8(text.c_str());
}

void fill(juce::ComboBox& box, const std::vector<std::string>& items)
{
    box.clear(juce::dontSendNotification);
    int id = 1;
    for (const auto& item : items)
        box.addItem(fromUtf8(item), id++);
}

/// One waveform lane: the processed signal or the DI signal of the active take.
class WaveformLane final : public juce::Component, private juce::ChangeListener
{
public:
    WaveformLane(TrackView& view, juce::String caption)
        : thumbnail_(kThumbnailResolution, view.formats(), view.thumbnails()), caption_(std::move(caption))
    {
        thumbnail_.addChangeListener(this);
    }

    ~WaveformLane() override { thumbnail_.removeChangeListener(this); }

    WaveformLane(const WaveformLane&) = delete;
    WaveformLane& operator=(const WaveformLane&) = delete;
    WaveformLane(WaveformLane&&) = delete;
    WaveformLane& operator=(WaveformLane&&) = delete;

    void show(const juce::File& file, juce::Colour colour)
    {
        colour_ = colour;
        if (file == file_)
            return;
        file_ = file;
        if (file == juce::File{})
            thumbnail_.clear();
        else
            thumbnail_.setSource(new juce::FileInputSource(
                file)); // NOLINT(cppcoreguidelines-owning-memory): JUCE takes ownership
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(theme::colours().gap);
        g.fillRect(getLocalBounds());
        g.setColour(theme::colours().textDim);
        g.setFont(juce::FontOptions{12.0F});
        g.drawText(caption_, getLocalBounds().reduced(kPadding, 2), juce::Justification::topLeft);
        if (thumbnail_.getTotalLength() <= 0.0)
            return;
        g.setColour(colour_);
        thumbnail_.drawChannel(g, getLocalBounds(), 0.0, thumbnail_.getTotalLength(), 0, 1.0F);
    }

private:
    void changeListenerCallback(juce::ChangeBroadcaster* /*source*/) override { repaint(); }

    juce::AudioThumbnail thumbnail_;
    juce::File file_;
    juce::String caption_;
    juce::Colour colour_;
};

/// Meter of one input; a click clears the clip marker.
class InputMeterBar final : public juce::Component
{
public:
    explicit InputMeterBar(TrackView& view) : view_(view) {}

    void setInput(int input) { input_ = input; }

    void paint(juce::Graphics& g) override
    {
        g.setColour(theme::colours().widget);
        g.fillRect(getLocalBounds());
        if (input_ < 0)
            return;
        const auto& level = view_.level(input_);
        auto area = getLocalBounds().toFloat();
        const auto clipArea = area.removeFromTop(static_cast<float>(kMeterWidth));
        if (level.clipped())
        {
            g.setColour(theme::colours().record);
            g.fillRect(clipArea);
        }
        const auto bar = area.removeFromBottom(area.getHeight() * level.position());
        g.setGradientFill(juce::ColourGradient::vertical(
            juce::Colour{0xFFE8B021}, area.getY(), juce::Colour{0xFF2FBF5B}, area.getBottom()));
        g.fillRect(bar);
    }

    void mouseUp(const juce::MouseEvent& /*event*/) override
    {
        if (input_ >= 0)
            view_.resetClip(input_);
    }

private:
    TrackView& view_;
    int input_ = -1;
};

juce::Colour voiceColour(std::size_t index)
{
    static const std::array kColours{juce::Colour{0xFF38A8E0},
                                     juce::Colour{0xFFF59E0B},
                                     juce::Colour{0xFF2FBF5B},
                                     juce::Colour{0xFFC060E0},
                                     juce::Colour{0xFFE05A5A}};
    return kColours.at(index % kColours.size());
}

} // namespace

/// One voice of the track view.
class TrackRowComponent final : public juce::Component
{
public:
    explicit TrackRowComponent(TrackView& view)
        : view_(view), diMeter_(view), wetMeter_(view), wetLane_(view, i18n::trJuce("processed")),
          diLane_(view, i18n::trJuce("DI"))
    {
        setUpHeader();
        setUpButtons();
        for (auto* component : std::initializer_list<juce::Component*>{&name_,
                                                                       &remove_,
                                                                       &profile_,
                                                                       &take_,
                                                                       &deleteTake_,
                                                                       &diCaption_,
                                                                       &wetCaption_,
                                                                       &di_,
                                                                       &wet_,
                                                                       &arm_,
                                                                       &mute_,
                                                                       &solo_,
                                                                       &gain_,
                                                                       &diMeter_,
                                                                       &wetMeter_,
                                                                       &wetLane_,
                                                                       &diLane_})
            addAndMakeVisible(component);
    }

    [[nodiscard]] const std::string& id() const { return id_; }

    void show(const ui::TrackRow& row,
              const std::vector<std::string>& inputs,
              const std::optional<std::filesystem::path>& folder,
              juce::Colour colour)
    {
        id_ = row.id;
        colour_ = colour;
        name_.setText(fromUtf8(row.name), juce::dontSendNotification);
        profile_.setSelectedItemIndex(row.profile, juce::dontSendNotification);
        fill(take_, row.takes);
        take_.setSelectedItemIndex(row.activeTake, juce::dontSendNotification);
        deleteTake_.setEnabled(row.activeTake >= 0);
        fill(di_, inputs);
        fill(wet_, inputs);
        di_.setSelectedItemIndex(row.diChoice, juce::dontSendNotification);
        wet_.setSelectedItemIndex(row.wetChoice, juce::dontSendNotification);
        arm_.setToggleState(row.armed, juce::dontSendNotification);
        mute_.setToggleState(row.muted, juce::dontSendNotification);
        solo_.setToggleState(row.solo, juce::dontSendNotification);
        if (!gain_.isMouseButtonDown())
            gain_.setValue(row.gain, juce::dontSendNotification);
        diMeter_.setInput(row.diChoice - 1);
        wetMeter_.setInput(row.wetChoice - 1);
        diLane_.show(fileOf(folder, row.diFile), colour);
        wetLane_.show(fileOf(folder, row.wetFile), colour);
        repaint();
    }

    void repaintMeters()
    {
        diMeter_.repaint();
        wetMeter_.repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.setColour(theme::colours().panel);
        g.fillRect(getLocalBounds());
        g.setColour(colour_);
        g.fillRect(getLocalBounds().removeFromLeft(3).withTrimmedBottom(1));
        g.setColour(theme::colours().outline);
        g.drawHorizontalLine(getHeight() - 1, 0.0F, static_cast<float>(getWidth()));
    }

    void resized() override
    {
        auto area = getLocalBounds().withTrimmedBottom(1);
        layOutHeader(area.removeFromLeft(kHeaderWidth).reduced(kPadding * 2, kPadding));

        auto meters = area.removeFromLeft(2 * kMeterWidth + 3 * 2).reduced(2, kPadding);
        diMeter_.setBounds(meters.removeFromLeft(kMeterWidth));
        meters.removeFromLeft(2);
        wetMeter_.setBounds(meters.removeFromLeft(kMeterWidth));
        area.removeFromLeft(2);
        wetLane_.setBounds(area.removeFromTop(area.getHeight() / 2).withTrimmedBottom(1));
        diLane_.setBounds(area);
    }

private:
    void setUpHeader()
    {
        name_.setEditable(false, true, false);
        name_.setFont(juce::FontOptions{15.0F, juce::Font::bold});
        name_.onTextChange = [this] { editor().rename(id_, name_.getText().toStdString()); };
        remove_.setButtonText(juce::String::fromUTF8("×"));
        remove_.setTooltip(i18n::trJuce("Remove voice"));
        remove_.onClick = [this] { editor().removeVoice(id_); };
        fill(profile_, ui::instrumentProfileChoices());
        profile_.onChange = [this] { editor().chooseProfile(id_, profile_.getSelectedItemIndex()); };
        take_.setTextWhenNothingSelected(i18n::trJuce("no take"));
        take_.onChange = [this] { editor().chooseTake(id_, take_.getSelectedItemIndex()); };
        deleteTake_.setButtonText(juce::String::fromUTF8("−"));
        deleteTake_.setTooltip(i18n::trJuce("Delete take"));
        deleteTake_.onClick = [this] { editor().deleteActiveTake(id_); };
        diCaption_.setText(i18n::trJuce("DI input"), juce::dontSendNotification);
        wetCaption_.setText(i18n::trJuce("Processed input"), juce::dontSendNotification);
        for (auto* caption : {&diCaption_, &wetCaption_})
            caption->setFont(juce::FontOptions{kCaptionHeight - 1.0F});
        di_.setTooltip(i18n::trJuce("Input of the dry signal straight from the instrument (DI box)"));
        wet_.setTooltip(i18n::trJuce("Input of the processed signal"));
        di_.onChange = [this] { chooseInputs(); };
        wet_.onChange = [this] { chooseInputs(); };
    }

    void setUpButtons()
    {
        arm_.setButtonText("R");
        arm_.setTooltip(i18n::trJuce("Arm for recording"));
        arm_.setClickingTogglesState(true);
        arm_.setColour(juce::TextButton::buttonOnColourId, theme::colours().record);
        arm_.onClick = [this] { view_.context().recording.setArmed(id_, arm_.getToggleState()); };
        mute_.setButtonText("M");
        mute_.setTooltip(i18n::trJuce("Mute"));
        mute_.setClickingTogglesState(true);
        mute_.onClick = [this] { editor().setMuted(id_, mute_.getToggleState()); };
        solo_.setButtonText("S");
        solo_.setTooltip(i18n::trJuce("Solo"));
        solo_.setClickingTogglesState(true);
        solo_.onClick = [this] { editor().setSolo(id_, solo_.getToggleState()); };
        gain_.setSliderStyle(juce::Slider::LinearHorizontal);
        gain_.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        gain_.setRange(0.0, 2.0);
        gain_.setTooltip(i18n::trJuce("Volume"));
        // One undo step per drag, not per pixel.
        gain_.onDragEnd = [this] { editor().setGain(id_, gain_.getValue()); };
    }

    void layOutHeader(juce::Rectangle<int> header)
    {
        auto line = header.removeFromTop(kLineHeight);
        remove_.setBounds(line.removeFromRight(kSmallButton));
        name_.setBounds(line);
        header.removeFromTop(2);
        line = header.removeFromTop(kLineHeight);
        const int half = (line.getWidth() - kPadding) / 2;
        profile_.setBounds(line.removeFromLeft(half));
        line.removeFromLeft(kPadding);
        deleteTake_.setBounds(line.removeFromRight(kSmallButton));
        take_.setBounds(line.withTrimmedRight(2));
        header.removeFromTop(2);
        auto caption = header.removeFromTop(kCaptionHeight);
        diCaption_.setBounds(caption.removeFromLeft(half));
        caption.removeFromLeft(kPadding);
        wetCaption_.setBounds(caption);
        line = header.removeFromTop(kLineHeight);
        di_.setBounds(line.removeFromLeft(half));
        line.removeFromLeft(kPadding);
        wet_.setBounds(line);
        header.removeFromTop(kPadding);
        line = header.removeFromTop(kLineHeight);
        for (auto* button : {&arm_, &mute_, &solo_})
        {
            button->setBounds(line.removeFromLeft(kSmallButton));
            line.removeFromLeft(4);
        }
        gain_.setBounds(line.withTrimmedLeft(kPadding));
    }

    ui::TrackEditor& editor() { return view_.context().editor; }

    void chooseInputs()
    {
        editor().chooseInputs(id_, di_.getSelectedItemIndex(), std::max(0, wet_.getSelectedItemIndex()));
    }

    static juce::File fileOf(const std::optional<std::filesystem::path>& folder, const std::string& relative)
    {
        if (!folder || relative.empty())
            return {};
        return juce::File{fromUtf8(io::utf8FromPath(*folder / io::pathFromUtf8(relative)))};
    }

    TrackView& view_;
    std::string id_;
    juce::Colour colour_;
    juce::Label name_;
    juce::TextButton remove_;
    juce::ComboBox profile_;
    juce::ComboBox take_;
    juce::TextButton deleteTake_;
    theme::DimLabel diCaption_;
    theme::DimLabel wetCaption_;
    juce::ComboBox di_;
    juce::ComboBox wet_;
    juce::TextButton arm_;
    juce::TextButton mute_;
    juce::TextButton solo_;
    juce::Slider gain_;
    InputMeterBar diMeter_;
    InputMeterBar wetMeter_;
    WaveformLane wetLane_;
    WaveformLane diLane_;
};

TrackView::TrackView(TrackViewContext context)
    : context_(std::move(context)), thumbnails_(kThumbnailCacheSize),
      lastTick_(juce::Time::getMillisecondCounterHiRes()),
      projectListener_(context_.project, [this] { triggerAsyncUpdate(); })
{
    formats_.registerBasicFormats();
    viewport_.setViewedComponent(&content_, false);
    viewport_.setScrollBarsShown(true, false);
    empty_.setText(i18n::trJuce("No voices yet. Add one with \"+ Voice\"."), juce::dontSendNotification);
    empty_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(viewport_);
    addChildComponent(empty_);
    handleAsyncUpdate();
    startTimer(kMeterIntervalMs);
}

TrackView::~TrackView()
{
    stopTimer();
    cancelPendingUpdate();
}

void TrackView::paint(juce::Graphics& g)
{
    g.fillAll(theme::colours().panel);
}

void TrackView::resized()
{
    viewport_.setBounds(getLocalBounds());
    empty_.setBounds(getLocalBounds());
    const int width = viewport_.getMaximumVisibleWidth();
    content_.setSize(width, kRowHeight * static_cast<int>(rows_.size()));
    int y = 0;
    for (auto& row : rows_)
    {
        row->setBounds(0, y, width, kRowHeight);
        y += kRowHeight;
    }
}

const ui::LevelMeter& TrackView::level(int input) const
{
    return levels_.at(static_cast<std::size_t>(input));
}

void TrackView::resetClip(int input)
{
    levels_.at(static_cast<std::size_t>(input)).resetClip();
}

void TrackView::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double seconds = (now - lastTick_) / kMillisecondsPerSecond;
    lastTick_ = now;
    for (int input = 0; input < engine::InputMeter::kMaxInputs; ++input)
        levels_.at(static_cast<std::size_t>(input)).update(context_.meter.takePeak(input), seconds);
    for (auto& row : rows_)
        row->repaintMeters();
}

void TrackView::handleAsyncUpdate()
{
    const model::Project project{context_.project, nullptr};
    const auto rows =
        ui::trackRows(project, [this](const std::string& id) { return context_.recording.isArmed(id); });
    const auto inputs = ui::inputChoices(context_.inputNames());
    const auto folder = context_.projectFolder();
    while (rows_.size() > rows.size())
        rows_.pop_back();
    while (rows_.size() < rows.size())
    {
        rows_.push_back(std::make_unique<TrackRowComponent>(*this));
        content_.addAndMakeVisible(*rows_.back());
    }
    for (std::size_t index = 0; index < rows.size(); ++index)
        rows_[index]->show(rows[index], inputs, folder, voiceColour(index));
    empty_.setVisible(rows.empty());
    resized();
}

} // namespace snotes::app
