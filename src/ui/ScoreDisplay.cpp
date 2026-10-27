#include "ui/ScoreDisplay.h"

#include "io/Utf8Path.h"
#include "notation/ScoreMidi.h"
#include "notation/SvgForJuce.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
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

std::optional<double> scoreQuarterAt(const juce::ValueTree& project, double seconds)
{
    const model::Project model{project, nullptr};
    const auto grid = model.beatGrid();
    const auto samples = grid.beats();
    const bool scored = !grid.scoreFile().empty();
    if (samples.size() < 2 || !scored)
        return std::nullopt;
    std::vector<double> beats;
    beats.reserve(samples.size());
    for (const auto sample : samples)
        beats.push_back(static_cast<double>(sample) / model.sampleRate());
    return notation::beatAt(beats, seconds) - grid.scoreOrigin();
}

Area unite(const Area& a, const Area& b)
{
    if (b.empty())
        return a;
    if (a.empty())
        return b;
    return {.left = std::min(a.left, b.left),
            .top = std::min(a.top, b.top),
            .right = std::max(a.right, b.right),
            .bottom = std::max(a.bottom, b.bottom)};
}

std::optional<Area> cursorArea(const std::vector<DrawnNote>& notes, double tick)
{
    std::optional<std::int64_t> latest;
    std::int64_t end = 0;
    for (const auto& note : notes)
    {
        end = std::max(end, note.span.end);
        const bool started = static_cast<double>(note.span.start) <= tick;
        if (started && note.span.start >= latest.value_or(note.span.start))
            latest = note.span.start;
    }
    if (!latest || tick >= static_cast<double>(end))
        return std::nullopt;
    Area column;
    Area bars;
    for (const auto& note : notes)
        if (note.span.start == *latest)
        {
            column = unite(column, note.area);
            bars = unite(bars, note.bar);
        }
    return Area{.left = column.left, .top = bars.top, .right = column.right, .bottom = bars.bottom};
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

std::optional<notation::WrittenNote> noteAt(const std::vector<DrawnNote>& notes, float x, float y)
{
    const DrawnNote* best = nullptr;
    float bestSize = 0.0F;
    for (const auto& note : notes)
    {
        const auto& area = note.area;
        if (x < area.left || x > area.right || y < area.top || y > area.bottom)
            continue;
        const auto size = (area.right - area.left) * (area.bottom - area.top);
        if (best == nullptr || size < bestSize)
        {
            best = &note;
            bestSize = size;
        }
    }
    if (best == nullptr)
        return std::nullopt;
    return best->written;
}

std::vector<Area> noteAreas(const std::vector<DrawnNote>& notes, const notation::WrittenNote& note)
{
    std::vector<Area> areas;
    for (const auto& drawn : notes)
    {
        const auto& written = drawn.written;
        if (written.part == note.part && written.pitch == note.pitch && written.seconds == note.seconds &&
            !drawn.area.empty())
            areas.push_back(drawn.area);
    }
    return areas;
}

} // namespace snotes::ui
