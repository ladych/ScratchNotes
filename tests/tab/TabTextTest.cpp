#include "tab/TabText.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace snotes::tab
{
namespace
{

const Fretboard kBass{.strings = {28, 33, 38, 43}, .frets = 24};

rhythm::QuantizedEvent event(rhythm::Fraction position, std::vector<int> pitches)
{
    rhythm::QuantizedEvent result;
    result.position = position;
    result.duration = rhythm::Fraction{1};
    result.pitches = std::move(pitches);
    return result;
}

TEST(TabTextTest, FTB01_ShapesNumberTheStringsFromTheHighestOne)
{
    EXPECT_EQ(shapeToText({{.string = 0, .fret = 3}, {.string = 3, .fret = 0}, {}}, 4), "4:3 1:0 -");
    EXPECT_EQ(shapeToText({}, 4), "");
}

TEST(TabTextTest, FTB01_NoEventsGiveEmptyStrings)
{
    EXPECT_EQ(tabText({}, {}, kBass, {}), "G ||\nD ||\nA ||\nE ||\n");
}

TEST(TabTextTest, FTB01_NotesWithGapsAndBars)
{
    // Two quarter notes in a bar of 2/4, then an eighth and a sixteenth in the next one.
    std::vector<rhythm::QuantizedEvent> events{
        event({0}, {28}), event({1}, {35}), event({2}, {38}), event({5, 2}, {38 + 12})};
    events.back().duration = rhythm::Fraction{1, 4};
    events[2].graces = {{36}};
    const std::vector<EventTab> tabs{
        {.notes = {{.string = 0, .fret = 0}}, .graces = {}},
        {.notes = {{.string = 1, .fret = 2}}, .graces = {}},
        {.notes = {{.string = 2, .fret = 0}}, .graces = {{{.string = 1, .fret = 3}}}},
        {.notes = {{.string = 2, .fret = 12}, {}}, .graces = {}},
    };
    EXPECT_EQ(tabText(events, tabs, kBass, {.beatsPerBar = 2, .phase = 0}),
              "G |--------|---------|\n"
              "D |--------|----0-12-|\n"
              "A |----2---|(3)------|\n"
              "E |0-------|---------|\n");
}

TEST(TabTextTest, FTB01_FourBarsPerSystem)
{
    std::vector<rhythm::QuantizedEvent> events;
    std::vector<EventTab> tabs;
    for (int bar = 0; bar < 5; ++bar)
    {
        events.push_back(event({bar}, {28}));
        tabs.push_back({.notes = {{.string = 0, .fret = 0}}, .graces = {}});
    }
    EXPECT_EQ(tabText(events, tabs, kBass, {.beatsPerBar = 1, .phase = 0}),
              "G |----|----|----|----|\n"
              "D |----|----|----|----|\n"
              "A |----|----|----|----|\n"
              "E |0---|0---|0---|0---|\n"
              "\n"
              "G |----|\n"
              "D |----|\n"
              "A |----|\n"
              "E |0---|\n");
}

TEST(TabTextTest, FTB01_ThePhaseMovesTheBarLines)
{
    const std::vector<rhythm::QuantizedEvent> events{event({0}, {28}), event({1}, {28})};
    const std::vector<EventTab> tabs(2, {.notes = {{.string = 0, .fret = 0}}, .graces = {}});
    EXPECT_EQ(tabText(events, tabs, kBass, {.beatsPerBar = 4, .phase = 1}),
              "G |----|----|\n"
              "D |----|----|\n"
              "A |----|----|\n"
              "E |0---|0---|\n");
}

TEST(TabTextTest, FTB01_LongGapsAreCut)
{
    const std::vector<rhythm::QuantizedEvent> events{event({0}, {28}), event({8}, {28})};
    const std::vector<EventTab> tabs(2, {.notes = {{.string = 0, .fret = 0}}, .graces = {}});
    const auto text = tabText(events, tabs, kBass, {.beatsPerBar = 16, .phase = 0});
    EXPECT_EQ(text.substr(text.rfind("E |")), "E |0---------------0---|\n");
}

} // namespace
} // namespace snotes::tab
