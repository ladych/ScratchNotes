#pragma once

#include "model/Project.h"
#include "notation/MusicXml.h"
#include "notation/ScoreRenderer.h"

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// Widths of the track view and the score view side by side (GUI-Entwurf v0.1, item 2); a view that is not
/// shown gets 0.
struct ViewWidths
{
    int tracks = 0;
    int notes = 0;

    bool operator==(const ViewWidths&) const = default;
};

/// Share of the width the tracks get when both views are shown.
inline constexpr double kTrackShare = 0.5;

/// Splits width between the views, with gap between them when both are shown.
[[nodiscard]] ViewWidths viewWidths(model::Project::View view, int width, int gap);

/// The score file of the project (F-NA-02), if it has one and the project has a folder.
[[nodiscard]] std::optional<std::filesystem::path>
scorePath(const juce::ValueTree& project, const std::optional<std::filesystem::path>& folder);

/// Where the song is in the score at a time in seconds (F-NA-04), in quarter notes from its start; none
/// without a grid or a score.
[[nodiscard]] std::optional<double> scoreQuarterAt(const juce::ValueTree& project, double seconds);

/// A rectangle in the pixels of the score view; empty without width or height.
struct Area
{
    float left = 0.0F;
    float top = 0.0F;
    float right = 0.0F;
    float bottom = 0.0F;

    [[nodiscard]] bool empty() const { return right <= left || bottom <= top; }
    bool operator==(const Area&) const = default;
};

/// The smallest area around both; an empty one adds nothing.
[[nodiscard]] Area unite(const Area& a, const Area& b);

/// A note as drawn in the score view, with its bar, in the view's pixels (F-NA-04).
struct DrawnNote
{
    notation::NoteSpan span;
    Area area;
    Area bar;
};

/// The cursor at a tick of the score: over the notes that started last, as high as their bars; none before
/// the first note and from the end of the last one on. Notes drawn without an area (frets on the tablature)
/// give it no width.
[[nodiscard]] std::optional<Area> cursorArea(const std::vector<DrawnNote>& notes, double tick);

/// The pages of a score as SVG that juce::Drawable can draw, pageWidth pixels wide, in the colour ink
/// ("#rrggbb"); none if the text is no score.
[[nodiscard]] std::vector<std::unique_ptr<juce::XmlElement>> scorePages(notation::ScoreRenderer& renderer,
                                                                        const std::string& musicXml,
                                                                        int pageWidth,
                                                                        const std::string& ink);

} // namespace snotes::ui
