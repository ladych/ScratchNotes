#pragma once

#include "ui/AboutInfo.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace snotes::app
{

/// Humble functions (E-03) opening the non-modal dialogs of the menus.

/// Audio settings: driver, device, sample rate, buffer size, inputs and outputs (F-AO-01).
void showAudioSettingsDialog(juce::AudioDeviceManager& deviceManager, juce::Component* centreAround);

/// Info: program, license, source code and third-party components with their licenses (Q-12).
void showAboutDialog(const ui::AboutInfo& info, juce::Component* centreAround);

} // namespace snotes::app
