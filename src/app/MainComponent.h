#pragma once

#include "app/AudioDeviceSupervisor.h"
#include "app/ChannelList.h"
#include "app/Theme.h"
#include "app/TrackView.h"
#include "engine/TestToneSource.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace snotes::app
{

/// Humble object (E-03): the main window. A top bar with Rec and the recording time, the test tone, the
/// audio settings and "+ Voice"; the track view (AP2); the inputs and outputs of the open device (F-AO-01)
/// and the status line with driver, buffer and latency (Q-02). The score view (AP7) takes the place of the
/// channel lists.
class MainComponent final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    MainComponent(juce::AudioDeviceManager& deviceManager,
                  const AudioDeviceSupervisor& deviceSupervisor,
                  engine::TestToneSource& testTone,
                  TrackViewContext tracks,
                  std::function<void()> showAudioSettings,
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
    void setUpTopBar(std::function<void()> showAudioSettings, std::function<void()> addVoice);
    void showChannels();
    void showRecordingState();

    juce::AudioDeviceManager& deviceManager_;
    const AudioDeviceSupervisor& deviceSupervisor_;
    engine::TestToneSource& testTone_;
    juce::TextButton testToneButton_;
    juce::TextButton settingsButton_;
    juce::TextButton recordButton_;
    juce::Label recordTime_;
    juce::TextButton addVoiceButton_;
    ui::RecordingController& recording_;
    TrackView tracks_;
    ChannelList inputs_;
    ChannelList outputs_;
    theme::DimLabel status_;
};

} // namespace snotes::app
