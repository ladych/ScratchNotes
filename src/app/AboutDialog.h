#pragma once

#include "ui/AboutInfo.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace snotes::app
{

/// Humble object (E-03): info dialog with the program, its license, its source code and the third-party
/// components with their licenses and clickable links (Q-12). The content comes from ui::makeAboutInfo.
class AboutDialog final : public juce::Component
{
public:
    explicit AboutDialog(const ui::AboutInfo& info);

    void resized() override;

private:
    /// Adds a text line; bold for titles.
    void addText(const juce::String& text, float fontHeight, bool bold);
    void addLink(const ui::Link& link);
    void addComponent(const ui::ThirdPartyComponent& component);
    void addGap();

    juce::Viewport viewport_;
    juce::Component content_;
    /// Items in display order; nullptr is a gap.
    std::vector<std::unique_ptr<juce::Component>> items_;
    int contentHeight_ = 0;

    JUCE_LEAK_DETECTOR(AboutDialog)
};

} // namespace snotes::app
