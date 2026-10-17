#pragma once

#include "engine/InputMeter.h"
#include "model/TreeChangeListener.h"
#include "ui/LevelMeter.h"
#include "ui/RecordingController.h"
#include "ui/TrackList.h"
#include "ui/TransportControl.h"

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
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
    ui::TransportControl& transport;
    engine::InputMeter& meter;
    /// Names of the inputs of the open device.
    std::function<std::vector<std::string>()> inputNames;
    /// Folder of the project, if it has one yet.
    std::function<std::optional<std::filesystem::path>()> projectFolder;
};

class TrackRowComponent;

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

private:
    void timerCallback() override;
    void handleAsyncUpdate() override;
    void updateTimeline();

    TrackViewContext context_;
    juce::AudioFormatManager formats_;
    juce::AudioThumbnailCache thumbnails_;
    std::array<ui::LevelMeter, engine::InputMeter::kMaxInputs> levels_{};
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
