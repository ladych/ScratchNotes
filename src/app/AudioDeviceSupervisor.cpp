#include "app/AudioDeviceSupervisor.h"

#include <functional>
#include <string>

namespace snotes::app
{
namespace
{

constexpr int kWatchIntervalMs = 1000;

} // namespace

AudioDeviceSupervisor::AudioDeviceSupervisor(juce::AudioDeviceManager& deviceManager,
                                             const io::SettingsStore& deviceSettings,
                                             const AudioCallback& audioCallback,
                                             ChannelCounts channels)
    : deviceManager_(deviceManager), deviceSettings_(deviceSettings), audioCallback_(audioCallback),
      channels_(channels)
{
}

AudioDeviceSupervisor::~AudioDeviceSupervisor()
{
    stopTimer();
}

void AudioDeviceSupervisor::open()
{
    const auto savedState = deviceSettings_.load();
    const auto saved = savedState ? juce::parseXML(juce::String::fromUTF8(savedState->c_str())) : nullptr;
    if (saved != nullptr)
    {
        // No JUCE default on failure: it would stay with the failed type, e.g. another ASIO driver.
        const auto error = deviceManager_.initialise(channels_.inputs, channels_.outputs, saved.get(), false);
        if (deviceManager_.getCurrentAudioDevice() == nullptr)
        {
            juce::Logger::writeToLog("Saved audio device could not be opened: " + error);
            openFallback(saved->getStringAttribute("deviceType"));
        }
    }
    else
    {
        // First start: the best driver type becomes the chosen one and is saved (F-AO-03).
        const auto types = audio::driverTypesByPreference(drivers());
        juce::XmlElement firstChoice("DEVICESETUP");
        if (!types.empty())
            firstChoice.setAttribute("deviceType", juce::String::fromUTF8(types.front().c_str()));
        const auto error = deviceManager_.initialise(channels_.inputs, channels_.outputs, &firstChoice, true);
        if (error.isNotEmpty())
            juce::Logger::writeToLog("Audio device could not be opened: " + error);
    }
    startTimer(kWatchIntervalMs);
}

void AudioDeviceSupervisor::describe(ui::AudioDeviceInfo& info) const
{
    info.stalled = recovery_.stalled();
    if (auto* device = deviceManager_.getCurrentAudioDevice())
    {
        info.xruns = device->getXRunCount();
        const auto chosen = chosenSetup();
        if (chosen != nullptr && !isOnChosenDevice(*chosen))
            info.replacedDevice = (chosen->getStringAttribute("deviceType") + ": " +
                                   chosen->getStringAttribute("audioOutputDeviceName"))
                                      .toStdString();
    }
}

void AudioDeviceSupervisor::timerCallback()
{
    const auto chosen = chosenSetup();
    std::string deviceList;
    for (const auto& driver : drivers())
    {
        deviceList += driver.type + '\n';
        for (const auto& input : driver.inputs)
            deviceList += "in " + input + '\n';
        for (const auto& output : driver.outputs)
            deviceList += output + '\n';
    }
    const audio::DeviceObservation observation{.hasPreferred = chosen != nullptr,
                                               .open = deviceManager_.getCurrentAudioDevice() != nullptr,
                                               .onPreferred = chosen != nullptr && isOnChosenDevice(*chosen),
                                               .audioBlocks = audioCallback_.blockCount(),
                                               .deviceList = std::hash<std::string>{}(deviceList)};
    if (recovery_.tick(observation) == audio::RecoveryAction::reopen)
        reopen();
}

void AudioDeviceSupervisor::reopen()
{
    const auto chosen = chosenSetup();
    juce::Logger::writeToLog("Reopening the audio device");
    deviceManager_.closeAudioDevice();
    if (chosen != nullptr)
    {
        deviceManager_.initialise(channels_.inputs, channels_.outputs, chosen.get(), false);
        if (deviceManager_.getCurrentAudioDevice() == nullptr)
            openFallback(chosen->getStringAttribute("deviceType"));
        return;
    }
    openFallback({});
}

void AudioDeviceSupervisor::openFallback(const juce::String& failedType)
{
    // Not "explicit" (false): the chosen device stays the saved one.
    for (const auto& type : audio::driverTypesByPreference(drivers(), failedType.toStdString()))
    {
        const auto typeName = juce::String::fromUTF8(type.c_str());
        deviceManager_.setCurrentAudioDeviceType(typeName, false);
        if (deviceManager_.getCurrentAudioDevice() == nullptr)
        {
            // Same type as before (no switch happened, e.g. the failed type as the last resort) or its
            // default device failed: open the default device directly.
            auto* deviceType = deviceManager_.getCurrentDeviceTypeObject();
            auto setup = deviceManager_.getAudioDeviceSetup();
            setup.inputDeviceName = deviceType->getDeviceNames(true)[deviceType->getDefaultDeviceIndex(true)];
            setup.outputDeviceName =
                deviceType->getDeviceNames(false)[deviceType->getDefaultDeviceIndex(false)];
            deviceManager_.setAudioDeviceSetup(setup, false);
        }
        if (deviceManager_.getCurrentAudioDevice() != nullptr)
        {
            juce::Logger::writeToLog("Audio fallback: " + typeName);
            return;
        }
    }
    juce::Logger::writeToLog("No audio device could be opened");
}

std::unique_ptr<juce::XmlElement> AudioDeviceSupervisor::chosenSetup() const
{
    auto setup = deviceManager_.createStateXml();
    if (setup != nullptr && setup->getStringAttribute("audioOutputDeviceName").isEmpty())
        return nullptr;
    return setup;
}

bool AudioDeviceSupervisor::isOnChosenDevice(const juce::XmlElement& setup) const
{
    const auto* device = deviceManager_.getCurrentAudioDevice();
    return device != nullptr && device->getTypeName() == setup.getStringAttribute("deviceType") &&
           device->getName() == setup.getStringAttribute("audioOutputDeviceName");
}

std::vector<audio::DriverDevices> AudioDeviceSupervisor::drivers()
{
    std::vector<audio::DriverDevices> result;
    for (const auto* type : deviceManager_.getAvailableDeviceTypes())
    {
        audio::DriverDevices driver{.type = type->getTypeName().toStdString(), .inputs = {}, .outputs = {}};
        for (const auto& name : type->getDeviceNames(true))
            driver.inputs.push_back(name.toStdString());
        for (const auto& name : type->getDeviceNames(false))
            driver.outputs.push_back(name.toStdString());
        result.push_back(std::move(driver));
    }
    return result;
}

} // namespace snotes::app
