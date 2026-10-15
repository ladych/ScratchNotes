#pragma once

#include "ui/ThemeSetting.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <vector>

namespace snotes::app::theme
{

/// Colour tokens of the main window, taken over from Drum Programmer (AP0, F-UI-04). Views take their colours
/// from colours(), never fixed values, so both themes look right.
struct Palette
{
    /// Gaps between the areas and the status line: they set the areas apart.
    juce::Colour gap;
    /// Background of an area (channel lists, later track view and score).
    juce::Colour panel;
    /// Widgets inside an area: buttons, combo boxes, text fields.
    juce::Colour widget;
    juce::Colour outline;
    juce::Colour text;
    juce::Colour textDim;
    /// Active state: running playback, chosen mode.
    juce::Colour accent;
    juce::Colour record;
    juce::Colour sliderFill;
    /// Slider groove and its filled part; thumb of the scroll bars.
    juce::Colour sliderTrack;
    juce::Colour sliderTrackFill;
    juce::Colour scrollThumb;
    /// Selected row of a list, highlighted menu entry.
    juce::Colour highlight;
    juce::Colour highlightedText;
};

inline const Palette kDark{.gap = juce::Colour{0xFF121418},
                           .panel = juce::Colour{0xFF1E2227},
                           .widget = juce::Colour{0xFF2A2F36},
                           .outline = juce::Colour{0xFF3A4048},
                           .text = juce::Colour{0xFFD6D9DD},
                           .textDim = juce::Colour{0xFF8A919A},
                           .accent = juce::Colour{0xFFF59E0B},
                           .record = juce::Colour{0xFFD03030},
                           .sliderFill = juce::Colour{0xFF38A8E0},
                           .sliderTrack = juce::Colour{0xFF2A2F36},
                           .sliderTrackFill = juce::Colour{0xFF2B5C8A},
                           .scrollThumb = juce::Colour{0xFF38A8E0},
                           .highlight = juce::Colour{0xFF2B5C8A},
                           .highlightedText = juce::Colours::white};

inline const Palette kLight{.gap = juce::Colour{0xFFE3E6EA},
                            .panel = juce::Colour{0xFFF8F9FA},
                            .widget = juce::Colours::white,
                            .outline = juce::Colour{0xFFCDD2D8},
                            .text = juce::Colour{0xFF1F2328},
                            .textDim = juce::Colour{0xFF6B7280},
                            .accent = juce::Colour{0xFFD97706},
                            .record = juce::Colour{0xFFDC2626},
                            .sliderFill = juce::Colour{0xFF0EA5E9},
                            .sliderTrack = juce::Colour{0xFFDDE1E6},
                            .sliderTrackFill = juce::Colour{0xFF38BDF8},
                            .scrollThumb = juce::Colour{0xFFB8C0CA},
                            .highlight = juce::Colour{0xFFDBEAFE},
                            .highlightedText = juce::Colour{0xFF1F2328}};

namespace detail
{
inline const Palette*& shown()
{
    static const Palette* palette = &kDark;
    return palette;
}
} // namespace detail

/// The palette of the theme shown now (LookAndFeel::setTheme). GUI thread only.
inline const Palette& colours()
{
    return *detail::shown();
}

/// JUCE look and feel with the colours of the shown theme.
class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel() { setTheme(ui::Theme::dark); }

    /// Switches the colours; components that set colours of their own refresh them in lookAndFeelChanged().
    void setTheme(ui::Theme theme)
    {
        const auto& palette = theme == ui::Theme::light ? kLight : kDark;
        detail::shown() = &palette;
        setColourScheme(ColourScheme{palette.panel,
                                     palette.widget,
                                     palette.widget,
                                     palette.outline,
                                     palette.text,
                                     palette.sliderFill,
                                     palette.highlightedText,
                                     palette.highlight,
                                     palette.text});
        setColour(juce::TooltipWindow::backgroundColourId, palette.widget);
        setColour(juce::TooltipWindow::textColourId, palette.text);
        setColour(juce::TooltipWindow::outlineColourId, palette.outline);
        setColour(juce::Slider::backgroundColourId, palette.sliderTrack);
        setColour(juce::Slider::trackColourId, palette.sliderTrackFill);
        setColour(juce::ScrollBar::thumbColourId, palette.scrollThumb);
    }
};

namespace detail
{
/// Text already in a text editor keeps the colour it was typed in, and applyColourToAllText() fixes the
/// editor's own text colour, so the colour comes from the look and feel.
inline void recolourText(juce::Component& window)
{
    std::vector<juce::Component*> pending{&window};
    while (!pending.empty())
    {
        auto* component = pending.back();
        pending.pop_back();
        if (component == nullptr)
            continue;
        if (auto* editor = dynamic_cast<juce::TextEditor*>(component))
            editor->applyColourToAllText(editor->getLookAndFeel().findColour(juce::TextEditor::textColourId));
        for (auto* child : component->getChildren())
            pending.push_back(child);
    }
}
} // namespace detail

/// Repaints every open window in the colours of the look and feel after LookAndFeel::setTheme().
inline void refreshWindows()
{
    for (int index = 0; index < juce::TopLevelWindow::getNumTopLevelWindows(); ++index)
    {
        auto* window = juce::TopLevelWindow::getTopLevelWindow(index);
        if (window == nullptr)
            continue;
        if (auto* resizable = dynamic_cast<juce::ResizableWindow*>(window))
            resizable->setBackgroundColour(
                resizable->getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
        detail::recolourText(*window);
        window->sendLookAndFeelChange();
    }
}

/// Label in the dimmed text colour, also after a change of the theme.
class DimLabel final : public juce::Label
{
public:
    explicit DimLabel(const juce::String& name = {}, const juce::String& text = {}) : juce::Label(name, text)
    {
        lookAndFeelChanged();
    }

    void lookAndFeelChanged() override { setColour(juce::Label::textColourId, colours().textDim); }
};

} // namespace snotes::app::theme
