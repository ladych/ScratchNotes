#include "ui/ScoreDisplay.h"

#include "io/Utf8Path.h"
#include "notation/SvgForJuce.h"

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

ViewWidths viewWidths(model::Project::View view, int width, int gap)
{
    if (view == model::Project::View::tracks)
        return {.tracks = width, .notes = 0};
    if (view == model::Project::View::notes)
        return {.tracks = 0, .notes = width};
    const auto tracks = static_cast<int>(std::lround((width - gap) * kTrackShare));
    return {.tracks = tracks, .notes = width - gap - tracks};
}

std::optional<std::filesystem::path> scorePath(const juce::ValueTree& project,
                                               const std::optional<std::filesystem::path>& folder)
{
    const auto file = model::Project{project, nullptr}.beatGrid().scoreFile();
    if (file.empty() || !folder)
        return std::nullopt;
    return *folder / io::pathFromUtf8(file);
}

std::vector<std::unique_ptr<juce::XmlElement>> scorePages(notation::ScoreRenderer& renderer,
                                                          const std::string& musicXml,
                                                          int pageWidth,
                                                          const std::string& ink)
{
    std::vector<std::unique_ptr<juce::XmlElement>> pages;
    renderer.setPageWidth(pageWidth);
    const auto count = renderer.load(musicXml).value_or(0);
    pages.reserve(static_cast<std::size_t>(count));
    for (int page = 1; page <= count; ++page)
        pages.push_back(notation::svgForJuce(renderer.page(page), ink));
    return pages;
}

} // namespace snotes::ui
