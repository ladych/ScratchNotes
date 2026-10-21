#include "rhythm/RhythmCsv.h"

#include <gtest/gtest.h>

namespace snotes::rhythm
{
namespace
{

TEST(RhythmCsvTest, FTX05_BeatsWithTheTempoToTheNextBeat)
{
    BeatGrid grid;
    grid.beats = {0.5, 1.0, 1.75};

    EXPECT_EQ(beatsToCsv(grid), "beat,seconds,bpm\n0,0.5000,120.00\n1,1.0000,80.00\n2,1.7500,\n");
}

TEST(RhythmCsvTest, FTX06_MeterSuggestionsMostLikelyFirst)
{
    BeatGrid grid;
    grid.meter.ranking = {{.beatsPerBar = 4, .confidence = 0.6884, .phase = 0},
                          {.beatsPerBar = 7, .confidence = 0.1017, .phase = 1}};

    EXPECT_EQ(meterToCsv(grid), "beats_per_bar,confidence,phase\n4,0.688,0\n7,0.102,1\n");
}

TEST(RhythmCsvTest, FTX09_FTB01_EventsAsFractionsOfBeatsWithTheirTab)
{
    VoiceRhythm rhythm;
    rhythm.quantization.events = {
        {.position = {0},
         .duration = {1, 2},
         .seconds = 0.25,
         .weight = 1.0,
         .pitches = {40, 47},
         .graces = {}},
        {.position = {4, 3},
         .duration = {2, 3},
         .seconds = 1.0,
         .weight = 0.5,
         .pitches = {45},
         .graces = {{43, 44}, {42}}},
    };

    const std::vector<tab::EventTab> tabs{
        {.notes = {{.string = 0, .fret = 0}, {.string = 1, .fret = 2}}, .graces = {}},
        {.notes = {{.string = 1, .fret = 0}},
         .graces = {{{.string = 0, .fret = 3}, {}}, {{.string = 0, .fret = 2}}}},
    };

    EXPECT_EQ(
        eventsToCsv(rhythm, tabs, 6),
        "position,duration,seconds,weight,pitches,graces,tab,grace_tab\n0,1/2,0.2500,1.0000,40 47,,6:0 5:2,\n"
        "4/3,2/3,1.0000,0.5000,45,43 44|42,5:0,6:3 -|6:2\n");
}

TEST(RhythmCsvTest, FTX04_OnsetsReadBackAsWritten)
{
    const Onsets onsets{.beat = {0.0F, 1.5F, 0.25F}, .detail = {0.125F, 2.0F}};

    EXPECT_EQ(onsetsToCsv(onsets), "hop,values\n441,0,1.5,0.25\n256,0.125,2\n");
    EXPECT_EQ(onsetsFromCsv(onsetsToCsv(onsets)), onsets);
    EXPECT_EQ(onsetsFromCsv(onsetsToCsv({})), Onsets{});
}

TEST(RhythmCsvTest, FTX04_RejectsOtherText)
{
    EXPECT_FALSE(onsetsFromCsv("").has_value());
    EXPECT_FALSE(onsetsFromCsv("hop,values\n441,1\n").has_value());
    EXPECT_FALSE(onsetsFromCsv("hop,values\n256,1\n441,1\n").has_value());
}

} // namespace
} // namespace snotes::rhythm
