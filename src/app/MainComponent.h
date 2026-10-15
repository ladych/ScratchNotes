#pragma once

#include "app/AudioDeviceSupervisor.h"
#include "app/ChannelList.h"
#include "app/Theme.h"
#include "engine/TestToneSource.h"

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace snotes::app
{

/// Humble object (E-03): the main window of milestone M0. A top bar with the test tone and the audio
/// settings, the inputs and outputs of the open device (F-AO-01) and the status line with driver, buffer and
/// latency (Q-02). The track view (AP2) and the score view (AP7) replace the channel lists.
class MainComponent final : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    MainComponent(juce::AudioDeviceManager& deviceManager,
                  const AudioDeviceSupervisor& deviceSupervisor,
                  engine::TestToneSource& testTone,
                  std::function<void()> showAudioSettings);
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
    void showChannels();

    juce::AudioDeviceManager& deviceManager_;
    const AudioDeviceSupervisor& deviceSupervisor_;
    engine::TestToneSource& testTone_;
    juce::TextButton testToneButton_;
    juce::TextButton settingsButton_;
    ChannelList inputs_;
    ChannelList outputs_;
    theme::DimLabel status_;
};

} // namespace snotes::app
