#pragma once

#include "ui/LatencyCalibration.h"
#include "ui/LatencyCompensation.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <string>
#include <vector>

namespace snotes::app
{

/// Humble object (E-03): the dialog "Latenzausgleich" (F-WI-05, Q-01, Q-05). Measures the round trip through
/// a loopback cable with ui::LatencyCalibration and lets the user set the offset by hand.
class LatencyDialog final : public juce::Component, private juce::Timer
{
public:
    LatencyDialog(ui::LatencyCalibration& calibration,
                  ui::LatencyCompensation& compensation,
                  const std::vector<std::string>& inputNames);
    ~LatencyDialog() override;

    LatencyDialog(const LatencyDialog&) = delete;
    LatencyDialog& operator=(const LatencyDialog&) = delete;
    LatencyDialog(LatencyDialog&&) = delete;
    LatencyDialog& operator=(LatencyDialog&&) = delete;

    void resized() override;

private:
    void timerCallback() override;
    void measure();
    void showCompensation();

    ui::LatencyCalibration& calibration_;
    ui::LatencyCompensation& compensation_;
    juce::Label help_;
    juce::Label inputLabel_;
    juce::ComboBox input_;
    juce::TextButton measure_;
    juce::Label result_;
    juce::Label offsetLabel_;
    juce::Slider offset_;
    juce::Label compensationText_;
};

} // namespace snotes::app
