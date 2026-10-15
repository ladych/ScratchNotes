#include "app/ChannelList.h"

#include "app/Theme.h"

#include <utility>

namespace snotes::app
{
namespace
{

constexpr int kHeadingHeight = 24;
constexpr int kRowHeight = 22;
constexpr int kTextIndent = 8;

} // namespace

ChannelList::ChannelList(const juce::String& heading)
{
    heading_.setText(heading, juce::dontSendNotification);
    heading_.setFont(juce::FontOptions{15.0F, juce::Font::bold});
    list_.setModel(this);
    list_.setRowHeight(kRowHeight);
    addAndMakeVisible(heading_);
    addAndMakeVisible(list_);
}

void ChannelList::show(std::vector<ui::ChannelEntry> entries)
{
    entries_ = std::move(entries);
    list_.updateContent();
    list_.repaint();
}

void ChannelList::resized()
{
    auto area = getLocalBounds();
    heading_.setBounds(area.removeFromTop(kHeadingHeight));
    list_.setBounds(area);
}

int ChannelList::getNumRows()
{
    return static_cast<int>(entries_.size());
}

void ChannelList::paintListBoxItem(
    int rowNumber, juce::Graphics& g, int width, int height, bool /*rowIsSelected*/)
{
    if (rowNumber < 0 || rowNumber >= getNumRows())
        return;
    const auto& entry = entries_[static_cast<std::size_t>(rowNumber)];
    g.setColour(entry.active ? theme::colours().text : theme::colours().textDim);
    g.drawText(juce::String::fromUTF8(entry.text.c_str()),
               kTextIndent,
               0,
               width - kTextIndent,
               height,
               juce::Justification::centredLeft);
}

} // namespace snotes::app
