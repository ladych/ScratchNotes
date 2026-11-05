#pragma once

#include "engine/InputMeter.h"
#include "model/TreeChangeListener.h"
#include "ui/BeatTapper.h"
#include "ui/GridCorrection.h"
#include "ui/LevelMeter.h"
#include "ui/NoteCorrection.h"
#include "ui/RecordingController.h"
#include "ui/RhythmUpdate.h"
#include "ui/TrackList.h"
#include "ui/TranscriptionController.h"
#include "ui/TransportControl.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace snotes::app
{

/// What the track view works with.
struct TrackViewContext
{
    juce::ValueTree project;
    ui::TrackEditor& editor;
    ui::RecordingController& recording;
    ui::TranscriptionController& transcription;
    ui::GridCorrection& grid;
    /// Corrections of single notes in the score view.
    ui::NoteCorrection& notes;
    ui::BeatTapper& tapper;
    ui::TransportControl& transport;
    engine::InputMeter& meter;
    /// Names of the inputs of the open device.
    std::function<std::vector<std::string>()> inputNames;
    /// Folder of the project, if it has one yet.
    std::function<std::optional<std::filesystem::path>()> projectFolder;
};

class TrackRowComponent;
class TrackView;

/// The beat grid above the waveforms (F-KO-02): a drag moves a beat, a double click inserts one, the context
/// menu deletes a beat or starts a bar on it; a click beside the beats moves the song position.
class BeatRuler final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit BeatRuler(TrackView& view);

    void paint(juce::Graphics& g) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

private:
    [[nodiscard]] double secondsAt(float x) const;
    [[nodiscard]] float xFor(double seconds) const;
    [[nodiscard]] std::optional<std::size_t> beatAt(float x) const;
    void showMenu(std::optional<std::size_t> beat, double seconds);

    TrackView& view_;
    std::optional<std::size_t> dragged_;
    float dragX_ = 0.0F;
};

/// Humble object (E-03): the track view (GUI-Entwurf v0.1, items 5, 6 and 8). One row per voice with name,
/// instrument profile, take choice, inputs, R/M/S/I, volume, input meters and the waveforms of the active
/// take on a common time line with the song position; a click into the waveforms moves the position. The
/// content comes from ui::trackRows, every edit goes to ui::TrackEditor, the RecordingController or the
/// TransportControl.
class TrackView final : public juce::Component, private juce::Timer, private juce::AsyncUpdater
{
public:
    explicit TrackView(TrackViewContext context);
    ~TrackView() override;

    TrackView(const TrackView&) = delete;
    TrackView& operator=(const TrackView&) = delete;
    TrackView(TrackView&&) = delete;
    TrackView& operator=(TrackView&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;
    /// The inputs of the device changed.
    void refresh() { triggerAsyncUpdate(); }

    [[nodiscard]] const ui::LevelMeter& level(int input) const;
    void resetClip(int input);
    [[nodiscard]] TrackViewContext& context() { return context_; }
    [[nodiscard]] juce::AudioFormatManager& formats() { return formats_; }
    [[nodiscard]] juce::AudioThumbnailCache& thumbnails() { return thumbnails_; }
    /// Seconds from the song start shown across the waveforms.
    [[nodiscard]] double visibleSeconds() const { return visibleSeconds_; }
    [[nodiscard]] double positionSeconds() const { return context_.transport.positionSeconds(); }
    /// The common beat grid drawn behind the waveforms (F-NA-03).
    [[nodiscard]] const std::vector<ui::BeatLine>& beatLines() const { return beatLines_; }
    /// Where the waveforms start in the width of the view.
    [[nodiscard]] static int lanesLeft();

private:
    void timerCallback() override;
    void handleAsyncUpdate() override;
    void updateTimeline();

    TrackViewContext context_;
    juce::AudioFormatManager formats_;
    juce::AudioThumbnailCache thumbnails_;
    std::array<ui::LevelMeter, engine::InputMeter::kMaxInputs> levels_{};
    std::vector<ui::BeatLine> beatLines_;
    BeatRuler ruler_;
    juce::Viewport viewport_;
    juce::Component content_;
    juce::Label empty_;
    std::vector<std::unique_ptr<TrackRowComponent>> rows_;
    double lastTick_ = 0.0;
    double visibleSeconds_ = 0.0;
    double shownPosition_ = -1.0;
    model::TreeChangeListener projectListener_;
};

} // namespace snotes::app
