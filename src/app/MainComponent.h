#pragma once

#include "app/AudioDeviceSupervisor.h"
#include "app/ScoreView.h"
#include "app/Theme.h"
#include "app/TrackView.h"
#include "model/Project.h"
#include "ui/TrackList.h"
#include "ui/TransportControl.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace snotes::app
{

/// Humble object (E-03): the main window. A top bar with "+" (add a voice), the transport (back to the
/// start, Play/Stop, Rec, position), the choice of the view (tracks, notes or both) and the click with its
/// tempo; the track view (AP2) and the score view (AP7) side by side and the status line with driver, buffer
/// and latency (Q-02) and the progress of the note detection (F-TX-01). Test tone, audio settings and latency
/// compensation are in the menu.
class MainComponent final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    MainComponent(juce::AudioDeviceManager& deviceManager,
                  const AudioDeviceSupervisor& deviceSupervisor,
                  TrackViewContext tracks,
                  std::function<void()> addVoice);
    ~MainComponent() override;

    MainComponent(const MainComponent&) = delete;
    MainComponent& operator=(const MainComponent&) = delete;
    MainComponent(MainComponent&&) = delete;
    MainComponent& operator=(MainComponent&&) = delete;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void timerCallback() override;
    void setUpTransport();
    void setUpClick();
    void setUpViews();
    void setUpMeter();
    void layOutBar(juce::Rectangle<int> bar);
    void showView();
    void showTransportState();
    void showClick();
    void showMeters();

    juce::AudioDeviceManager& deviceManager_;
    const AudioDeviceSupervisor& deviceSupervisor_;
    juce::TextButton addVoiceButton_;
    juce::TextButton startButton_;
    juce::TextButton playButton_;
    juce::TextButton recordButton_;
    juce::Label position_;
    juce::TextButton tracksButton_;
    juce::TextButton notesButton_;
    juce::TextButton bothButton_;
    juce::ComboBox meter_;
    std::vector<ui::MeterChoice> shownMeters_;
    juce::TextButton clickButton_;
    juce::Slider clickTempo_;
    ui::RecordingController& recording_;
    ui::TranscriptionController& transcription_;
    ui::TransportControl& transport_;
    ui::TrackEditor& editor_;
    ui::GridCorrection& grid_;
    juce::ValueTree project_;
    TrackView tracks_;
    ScoreView score_;
    model::Project::View shownView_ = model::Project::View::both;
    theme::DimLabel status_;
    juce::TooltipWindow tooltips_{this};
};

} // namespace snotes::app
