#pragma once

#include "model/TreeChangeListener.h"
#include "notation/ScoreRenderer.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace snotes::app
{

/// Humble object (E-03): the score view (GUI-Entwurf v0.1, item 9). Draws the project's score with Verovio
/// (F-NA-01, F-NA-02) page below page across its width in the colours of the theme, after every analysis and
/// when its width changes. A cursor follows the song position over the notes that sound (F-NA-04), and the
/// view scrolls along with it.
class ScoreView final : public juce::Component, private juce::AsyncUpdater, private juce::Timer
{
public:
    ScoreView(juce::ValueTree project,
              std::function<std::optional<std::filesystem::path>()> projectFolder,
              std::function<double()> positionSeconds);
    ~ScoreView() override;

    ScoreView(const ScoreView&) = delete;
    ScoreView& operator=(const ScoreView&) = delete;
    ScoreView(ScoreView&&) = delete;
    ScoreView& operator=(ScoreView&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

private:
    class Pages;

    void handleAsyncUpdate() override;
    void timerCallback() override;
    void showScore(const juce::File& file);

    juce::ValueTree project_;
    std::function<std::optional<std::filesystem::path>()> projectFolder_;
    std::function<double()> positionSeconds_;
    notation::ScoreRenderer renderer_;
    juce::Viewport viewport_;
    std::unique_ptr<Pages> pages_;
    juce::Label empty_;
    /// File, its time and the width of what is shown.
    juce::String shown_;
    model::TreeChangeListener projectListener_;
};

} // namespace snotes::app
