#include "tab/Tablature.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <vector>

namespace snotes::tab
{
namespace
{

const Fretboard kGuitar{.strings = {40, 45, 50, 55, 59, 64}, .frets = 24};
const Fretboard kBass{.strings = {28, 33, 38, 43}, .frets = 24};

Placement at(int string, int fret)
{
    return {.string = string, .fret = fret};
}

const Placement kUnplayable{};

TEST(TablatureTest, FTB01_FretboardComesFromTheProfile)
{
    const auto profile = model::findInstrumentProfile("bass5-gs");
    ASSERT_TRUE(profile.has_value());
    EXPECT_EQ(fretboardOf(*profile), (Fretboard{.strings = {20, 25, 30, 35, 40}, .frets = 24}));
}

TEST(TablatureTest, FTB01_ANoteHasAShapeOnEveryStringThatReachesIt)
{
    const std::vector<int> pitches{45};
    EXPECT_EQ(playableShapes(pitches, kGuitar), (std::vector<Shape>{{at(0, 5)}, {at(1, 0)}}));
}

TEST(TablatureTest, FTB01_ANoteOffTheFretboardIsUnplayable)
{
    const std::vector<int> low{39};
    EXPECT_EQ(playableShapes(low, kGuitar), (std::vector<Shape>{{kUnplayable}}));
    const std::vector<int> high{64 + 25};
    EXPECT_EQ(playableShapes(high, kGuitar), (std::vector<Shape>{{kUnplayable}}));
}

TEST(TablatureTest, FTB02_ChordsStayWithinTheStretch)
{
    // F1 only on the low string; C4 on the B string (1), the G string (5) or too far up the D or A string.
    const std::vector<int> pitches{41, 60};
    EXPECT_EQ(playableShapes(pitches, kGuitar),
              (std::vector<Shape>{{at(0, 1), at(3, 5)}, {at(0, 1), at(4, 1)}}));
}

TEST(TablatureTest, FTB02_OpenStringsDoNotStretch)
{
    const std::vector<int> pitches{40, 64 + 12};
    const auto shapes = playableShapes(pitches, kGuitar);
    ASSERT_FALSE(shapes.empty());
    EXPECT_EQ(shapes.back(), (Shape{at(0, 0), at(5, 12)}));
}

TEST(TablatureTest, FTB02_TwoNotesOnlyOneStringCanTakeLeaveTheFewestUnplayable)
{
    const std::vector<int> pitches{28, 29};
    EXPECT_EQ(playableShapes(pitches, kBass),
              (std::vector<Shape>{{at(0, 0), kUnplayable}, {kUnplayable, at(0, 1)}}));
}

TEST(TablatureTest, FTB02_MoreNotesThanStringsLeaveOneUnplayable)
{
    const std::vector<int> pitches{28, 33, 38, 43, 48};
    const auto shapes = playableShapes(pitches, kBass);
    ASSERT_FALSE(shapes.empty());
    for (const auto& shape : shapes)
        EXPECT_EQ(std::ranges::count(shape, kUnplayable), 1);
}

TEST(TablatureTest, FTB02_NoChordsNoFingering)
{
    EXPECT_TRUE(fingering({}, kGuitar).empty());
}

TEST(TablatureTest, FTB02_ALoneNoteTakesTheLowestPosition)
{
    // A3: G string 2 rather than D string 7 or A string 12.
    EXPECT_EQ(fingering({{.seconds = 0.0, .pitches = {57}}}, kGuitar), (std::vector<Shape>{{at(3, 2)}}));
}

TEST(TablatureTest, FTB02_TheHandStaysNearItsPosition)
{
    // An octave shape at the twelfth fret, then A4 quickly: B string 10 rather than high E string 5.
    const std::vector<Chord> chords{{.seconds = 0.0, .pitches = {71, 76}}, {.seconds = 0.2, .pitches = {69}}};
    const auto shapes = fingering(chords, kGuitar);
    ASSERT_EQ(shapes.size(), 2U);
    EXPECT_EQ(shapes[0], (Shape{at(4, 12), at(5, 12)}));
    EXPECT_EQ(shapes[1], (Shape{at(4, 10)}));
}

TEST(TablatureTest, FTB02_OpenStringsKeepTheHandWhereItIs)
{
    const std::vector<Chord> chords{{.seconds = 0.0, .pitches = {71, 76}},
                                    {.seconds = 1.0, .pitches = {40}},
                                    {.seconds = 2.0, .pitches = {69}}};
    const auto shapes = fingering(chords, kGuitar);
    ASSERT_EQ(shapes.size(), 3U);
    EXPECT_EQ(shapes[1], (Shape{at(0, 0)}));
    EXPECT_EQ(shapes[2], (Shape{at(4, 10)}));
}

TEST(TablatureTest, FTB02_ALongPauseMakesMovingCheaper)
{
    // F#2 on the low string, then F#2 with D#3: right away the hand stays and stretches to the A string 6;
    // after a pause it moves a fret down for the D string 1.
    const std::vector<Chord> quick{{.seconds = 0.0, .pitches = {42}}, {.seconds = 0.1, .pitches = {42, 51}}};
    const std::vector<Chord> paused{{.seconds = 0.0, .pitches = {42}}, {.seconds = 2.0, .pitches = {42, 51}}};
    EXPECT_EQ(fingering(quick, kGuitar).back(), (Shape{at(0, 2), at(1, 6)}));
    EXPECT_EQ(fingering(paused, kGuitar).back(), (Shape{at(0, 2), at(2, 1)}));
}

TEST(TablatureTest, FTB02_ScalesWalkAcrossTheStrings)
{
    // E minor pentatonic in the open position: every note on the lowest fret of a string.
    const std::vector<int> scale{40, 43, 45, 47, 50, 52, 55, 57, 59, 62, 64};
    std::vector<Chord> chords;
    for (std::size_t index = 0; index < scale.size(); ++index)
        chords.push_back({.seconds = 0.25 * static_cast<double>(index), .pitches = {scale[index]}});
    const auto shapes = fingering(chords, kGuitar);
    const std::vector<Shape> expected{{at(0, 0)},
                                      {at(0, 3)},
                                      {at(1, 0)},
                                      {at(1, 2)},
                                      {at(2, 0)},
                                      {at(2, 2)},
                                      {at(3, 0)},
                                      {at(3, 2)},
                                      {at(4, 0)},
                                      {at(4, 3)},
                                      {at(5, 0)}};
    EXPECT_EQ(shapes, expected);
}

TEST(TablatureTest, FTB02_OpenStringsInARowStayOpen)
{
    const std::vector<Chord> chords{{.seconds = 0.0, .pitches = {45}},
                                    {.seconds = 0.2, .pitches = {50}},
                                    {.seconds = 0.4, .pitches = {55}}};
    EXPECT_EQ(fingering(chords, kGuitar), (std::vector<Shape>{{at(1, 0)}, {at(2, 0)}, {at(3, 0)}}));
}

TEST(TablatureTest, FTB02_UnplayableNotesAreCarriedAlong)
{
    const std::vector<Chord> chords{{.seconds = 0.0, .pitches = {20}}, {.seconds = 0.5, .pitches = {45}}};
    EXPECT_EQ(fingering(chords, kGuitar), (std::vector<Shape>{{kUnplayable}, {at(1, 0)}}));
}

TEST(TablatureTest, FTB01_GraceNotesAreFingeredWithTheirNote)
{
    rhythm::QuantizedEvent event;
    event.seconds = 1.0;
    event.pitches = {57};
    event.graces = {{55}, {56}};
    const auto tabs = tabulate({event}, kGuitar);
    ASSERT_EQ(tabs.size(), 1U);
    EXPECT_EQ(tabs[0].notes, (Shape{at(3, 2)}));
    EXPECT_EQ(tabs[0].graces, (std::vector<Shape>{{at(3, 0)}, {at(3, 1)}}));
}

TEST(TablatureTest, FTB03_AFixedStringMovesTheRestOfTheWay)
{
    // A3 then C4: G string 2, then B string 1. With A3 fixed on the D string 7, C4 follows to the G string 5.
    rhythm::QuantizedEvent a;
    a.seconds = 0.0;
    a.pitches = {57};
    auto b = a;
    b.seconds = 0.2;
    b.pitches = {60};

    EXPECT_EQ(tabulate({a, b}, kGuitar)[1].notes, (Shape{at(4, 1)}));
    const auto fixed = tabulate({a, b}, kGuitar, {{2}});
    EXPECT_EQ(fixed[0].notes, (Shape{at(2, 7)}));
    EXPECT_EQ(fixed[1].notes, (Shape{at(3, 5)}));
}

TEST(TablatureTest, FTB03_AStringThatCannotPlayThePitchIsIgnored)
{
    // B2 has no place on the G string, so the fixed string is ignored.
    EXPECT_EQ(fingering({{.seconds = 0.0, .pitches = {47}, .strings = {3}}}, kGuitar),
              fingering({{.seconds = 0.0, .pitches = {47}}}, kGuitar));
    // Only the fixed pitch of a chord is bound, the other one finds its place.
    const auto chord =
        fingering({{.seconds = 0.0, .pitches = {45, 57}, .strings = {Placement::kUnplayable, 2}}}, kGuitar);
    EXPECT_EQ(chord.front()[1], at(2, 7));
}

TEST(TablatureTest, FTB01_NoEventsNoTablature)
{
    EXPECT_TRUE(tabulate({}, kGuitar).empty());
}

TEST(TablatureTest, FTB01_EventTabsCompareAllFields)
{
    const EventTab tab{.notes = {at(0, 0)}, .graces = {}};
    EXPECT_EQ(tab, (EventTab{.notes = {at(0, 0)}, .graces = {}}));
    EXPECT_NE(tab, (EventTab{.notes = {at(0, 1)}, .graces = {}}));
    EXPECT_NE(tab, (EventTab{.notes = {at(0, 0)}, .graces = {{at(0, 0)}}}));
    EXPECT_NE(at(0, 0), at(1, 0));
    EXPECT_NE(kGuitar, (Fretboard{.strings = kGuitar.strings, .frets = 22}));
    EXPECT_NE(kGuitar, kBass);
}

} // namespace
} // namespace snotes::tab
