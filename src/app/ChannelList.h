#pragma once

#include "ui/ChannelOverview.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace snotes::app
{

/// Humble object (E-03): the inputs or the outputs of the open device as a list with a heading (F-AO-01).
/// The lines come from ui::channelEntries; channels that are off are dimmed.
class ChannelList final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit ChannelList(const juce::String& heading);

    void show(std::vector<ui::ChannelEntry> entries);
    void resized() override;

private:
    int getNumRows() override;
    void
    paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;

    juce::Label heading_;
    juce::ListBox list_;
    std::vector<ui::ChannelEntry> entries_;
};

} // namespace snotes::app
