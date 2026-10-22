#pragma once

#include "model/Project.h"
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

/// The pages of a score as SVG that juce::Drawable can draw, pageWidth pixels wide, in the colour ink
/// ("#rrggbb"); none if the text is no score.
[[nodiscard]] std::vector<std::unique_ptr<juce::XmlElement>> scorePages(notation::ScoreRenderer& renderer,
                                                                        const std::string& musicXml,
                                                                        int pageWidth,
                                                                        const std::string& ink);

} // namespace snotes::ui
