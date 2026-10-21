#include "tab/TabText.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <ranges>
#include <string_view>

namespace snotes::tab
{
namespace
{

constexpr std::array<std::string_view, 12> kNoteNames{
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
constexpr int kBarsPerSystem = 4;
/// Dashes per beat of distance to the next note, at most kMostDashes.
constexpr double kDashesPerBeat = 4.0;
constexpr int kMostDashes = 16;

/// The text of a column on each string, empty where the column plays nothing.
using Column = std::vector<std::string>;

Column column(const Shape& shape, std::size_t strings, bool grace)
{
    Column cells(strings);
    for (const auto& placement : shape)
        if (placement.string != Placement::kUnplayable)
        {
            auto& cell = cells[static_cast<std::size_t>(placement.string)];
            cell = std::to_string(placement.fret);
            if (grace)
                cell.insert(0, "(").append(")");
        }
    return cells;
}

double beats(const rhythm::Fraction& fraction)
{
    return static_cast<double>(fraction.numerator()) / static_cast<double>(fraction.denominator());
}

int barOf(const rhythm::Fraction& position, const BarLines& bars)
{
    return static_cast<int>(std::floor((beats(position) - bars.phase) / bars.beatsPerBar));
}

struct Systems
{
    std::vector<std::string> lines;
    std::string text;
    int barsInSystem = 0;
};

void append(Systems& systems, const Column& cells, int width)
{
    for (std::size_t string = 0; string < cells.size(); ++string)
    {
        auto& line = systems.lines[cells.size() - 1 - string];
        line += cells[string];
        line.append(static_cast<std::size_t>(width) - cells[string].size(), '-');
    }
}

int widthOf(const Column& cells)
{
    std::size_t widest = 0;
    for (const auto& cell : cells)
        widest = std::max(widest, cell.size());
    return static_cast<int>(widest) + 1;
}

void startSystem(Systems& systems, const Fretboard& fretboard)
{
    systems.lines.clear();
    for (const int open : std::views::reverse(fretboard.strings))
    {
        std::string label{kNoteNames.at(static_cast<std::size_t>(open % 12))};
        label.resize(2, ' ');
        systems.lines.push_back(label + "|");
    }
    systems.barsInSystem = 0;
}

void endSystem(Systems& systems)
{
    if (!systems.text.empty())
        systems.text += '\n';
    for (const auto& line : systems.lines)
        systems.text += line + "|\n";
}

void barLine(Systems& systems, const Fretboard& fretboard)
{
    if (++systems.barsInSystem < kBarsPerSystem)
    {
        for (auto& line : systems.lines)
            line += '|';
        return;
    }
    endSystem(systems);
    startSystem(systems, fretboard);
}

} // namespace

std::string shapeToText(const Shape& shape, std::size_t strings)
{
    std::string text;
    for (const auto& placement : shape)
    {
        if (!text.empty())
            text += ' ';
        if (placement.string == Placement::kUnplayable)
        {
            text += '-';
            continue;
        }
        text += std::to_string(strings - static_cast<std::size_t>(placement.string));
        text += ':';
        text += std::to_string(placement.fret);
    }
    return text;
}

std::string tabText(const std::vector<rhythm::QuantizedEvent>& events,
                    const std::vector<EventTab>& tabs,
                    const Fretboard& fretboard,
                    const BarLines& bars)
{
    const auto strings = fretboard.strings.size();
    Systems systems;
    startSystem(systems, fretboard);
    for (std::size_t index = 0; index < events.size(); ++index)
    {
        const auto& event = events[index];
        if (index > 0 && barOf(event.position, bars) > barOf(events[index - 1].position, bars))
            barLine(systems, fretboard);
        for (const auto& grace : tabs[index].graces)
        {
            const auto cells = column(grace, strings, true);
            append(systems, cells, widthOf(cells));
        }
        const auto distance =
            index + 1 < events.size() ? events[index + 1].position - event.position : event.duration;
        const auto cells = column(tabs[index].notes, strings, false);
        const auto dashes =
            std::min(kMostDashes, static_cast<int>(std::lround(beats(distance) * kDashesPerBeat)));
        append(systems, cells, std::max(widthOf(cells), dashes));
    }
    endSystem(systems);
    return systems.text;
}

} // namespace snotes::tab
