#pragma once

#include "rhythm/Quantizer.h"
#include "tab/Tablature.h"

#include <cstddef>
#include <string>
#include <vector>

namespace snotes::tab
{

/// "6:0 5:2" with the strings numbered from 1 at the highest one as in tablature, "-" for an unplayable note.
[[nodiscard]] std::string shapeToText(const Shape& shape, std::size_t strings);

/// Where the bars of a meter start: every beatsPerBar beats from the beat phase on.
struct BarLines
{
    int beatsPerBar = 4;
    int phase = 0;
};

/// Plain-text tablature to check a fingering by eye (milestone M2): a line per string from the highest one,
/// labelled with its note, a column per note with more dashes for longer gaps, grace notes in brackets, bar
/// lines from the meter and four bars per system. tabs belong to events, one each.
[[nodiscard]] std::string tabText(const std::vector<rhythm::QuantizedEvent>& events,
                                  const std::vector<EventTab>& tabs,
                                  const Fretboard& fretboard,
                                  const BarLines& bars);

} // namespace snotes::tab
