#include "rhythm/RhythmAnalysis.h"

#include "rhythm/OnsetStrength.h"
#include "rhythm/RhythmReference.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace snotes::rhythm
{
namespace
{

using ::testing::DoubleNear;
using ::testing::ElementsAre;
using ::testing::Pointwise;

RhythmTrack referenceTrack(const std::string& prefix, double startSeconds = 0.0)
{
    return {.notes = test::readNotes(prefix),
            .onsets = {.beat = test::readFloats(prefix + "-oenv441.f32"),
                       .detail = test::readFloats(prefix + "-oenv256.f32")},
            .startSeconds = startSeconds};
}

class RhythmAnalysisReferenceTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(RhythmAnalysisReferenceTest, E12_TheWholePipelineMatchesThePoc)
{
    const auto track = referenceTrack(GetParam());
    const auto grid = commonBeatGrid({track}, kLibrosaWindow);

    ASSERT_TRUE(grid.has_value());
    EXPECT_THAT(grid->beats, Pointwise(DoubleNear(1e-6), test::readBeats(GetParam(), true)));
    EXPECT_EQ(grid->meter.ranking.front().beatsPerBar,
              std::stoi(test::readTextRows(GetParam() + "-meter.csv")[0][0]));
    const auto rhythm = voiceRhythm(track, *grid);
    const auto events = test::readTextRows(GetParam() + "-events.csv");
    ASSERT_EQ(rhythm.quantization.events.size(), events.size());
    for (std::size_t index = 0; index < events.size(); ++index)
        EXPECT_EQ(rhythm.quantization.events[index].position.toString(), events[index][0]);
}

TEST_P(RhythmAnalysisReferenceTest, FTX10_SameInputGivesTheSameResult)
{
    const auto track = referenceTrack(GetParam());
    const auto first = commonBeatGrid({track}, BeatTrackerSettings{});
    const auto second = commonBeatGrid({track}, BeatTrackerSettings{});

    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first, second);
    EXPECT_EQ(voiceRhythm(track, *first), voiceRhythm(track, *second));
}

INSTANTIATE_TEST_SUITE_P(Recordings, RhythmAnalysisReferenceTest, ::testing::Values("riff", "clean"));

TEST(RhythmAnalysisTest, FTX05_TracksShareOneGridInSongTime)
{
    const auto alone = commonBeatGrid({referenceTrack("riff")}, BeatTrackerSettings{});
    const auto later = referenceTrack("riff", 2.0);
    const auto shifted = commonBeatGrid({later}, BeatTrackerSettings{});

    ASSERT_TRUE(alone.has_value());
    ASSERT_TRUE(shifted.has_value());
    ASSERT_EQ(shifted->beats.size(), alone->beats.size());
    // The silence in front of the later track moves single beats by up to one frame of the tracker.
    for (std::size_t beat = 0; beat < alone->beats.size(); ++beat)
        EXPECT_NEAR(shifted->beats[beat], alone->beats[beat] + 2.0, 0.021);
    EXPECT_EQ(voiceRhythm(later, *shifted).quantization,
              voiceRhythm(referenceTrack("riff"), *alone).quantization);
}

TEST(RhythmAnalysisTest, FTX05_TheGridFollowsAllTracks)
{
    const auto both =
        commonBeatGrid({referenceTrack("riff"), referenceTrack("clean")}, BeatTrackerSettings{});

    ASSERT_TRUE(both.has_value());
    // The clean excerpt is ten seconds longer than the riff: the grid goes on to its end.
    EXPECT_GT(both->beats.back(), test::readNotes("riff").back().startSeconds + 9.0);
}

TEST(RhythmAnalysisTest, FTX04_NothingWithoutNotesOrBeats)
{
    auto track = referenceTrack("riff");
    auto silent = track;
    silent.onsets = {.beat = std::vector<float>(100), .detail = std::vector<float>(100)};
    auto noNotes = track;
    noNotes.notes.clear();
    auto oneBeat = track;
    oneBeat.notes = {{.startSeconds = 30.0, .endSeconds = 30.5, .pitch = 40, .amplitude = 0.8F}};

    EXPECT_FALSE(commonBeatGrid({noNotes}, BeatTrackerSettings{}).has_value());
    EXPECT_FALSE(commonBeatGrid({silent}, BeatTrackerSettings{}).has_value());
    EXPECT_FALSE(commonBeatGrid({oneBeat}, BeatTrackerSettings{}).has_value());
    EXPECT_FALSE(commonBeatGrid({}, BeatTrackerSettings{}).has_value());
}

TEST(RhythmAnalysisTest, FTX05_TheGridSpansTheNotes)
{
    auto late = referenceTrack("riff");
    std::erase_if(late.notes, [](const auto& note) { return note.startSeconds < 10.0; });
    auto early = referenceTrack("riff");
    std::erase_if(early.notes, [](const auto& note) { return note.startSeconds > 10.0; });

    const auto lateGrid = commonBeatGrid({late}, BeatTrackerSettings{});
    const auto earlyGrid = commonBeatGrid({early}, BeatTrackerSettings{});

    ASSERT_TRUE(lateGrid.has_value());
    ASSERT_TRUE(earlyGrid.has_value());
    EXPECT_GT(lateGrid->beats.front(), 9.0);
    EXPECT_LT(earlyGrid->beats.back(), 11.0);
}

TEST(RhythmAnalysisTest, FTX04_NothingWithASingleBeat)
{
    // One strong and one weak onset: the weak beat at the end is trimmed.
    RhythmTrack track{.notes = {{.startSeconds = 0.4, .endSeconds = 0.8, .pitch = 40, .amplitude = 0.8F}},
                      .onsets = {.beat = std::vector<float>(80), .detail = std::vector<float>(140)},
                      .startSeconds = 0.0};
    track.onsets.beat[20] = 10.0F;
    track.onsets.beat[50] = 1.0F;

    EXPECT_FALSE(commonBeatGrid({track}, BeatTrackerSettings{}).has_value());
}

TEST(RhythmAnalysisTest, FTX05_ResultsDifferInEveryField)
{
    const Onsets onsets{.beat = {1.0F}, .detail = {1.0F}};
    EXPECT_NE(onsets, (Onsets{.beat = {}, .detail = {1.0F}}));
    EXPECT_NE(onsets, (Onsets{.beat = {1.0F}, .detail = {}}));
    const BeatGrid grid{.tempoCandidates = {{.bpm = 100.0, .share = 1.0}}, .beats = {1.0}, .meter = {}};
    auto other = grid;
    other.tempoCandidates.clear();
    EXPECT_NE(other, grid);
    other = grid;
    other.beats.clear();
    EXPECT_NE(other, grid);
    other = grid;
    other.meter.phrase.beats = 4;
    EXPECT_NE(other, grid);
    const VoiceRhythm rhythm{.style = {.style = Style::riff, .chordShare = 1.0}, .quantization = {}};
    EXPECT_NE(rhythm, (VoiceRhythm{.style = {}, .quantization = {}}));
    EXPECT_NE(rhythm,
              (VoiceRhythm{.style = rhythm.style, .quantization = {.events = {}, .gridUse = {1, 0, 0, 0}}}));
}

TEST(RhythmAnalysisTest, FTX05_TempoCurveFromTheBeatIntervals)
{
    EXPECT_THAT(tempoCurve({1.0, 1.5, 2.25}), ElementsAre(DoubleNear(120.0, 1e-9), DoubleNear(80.0, 1e-9)));
    EXPECT_TRUE(tempoCurve({1.0}).empty());
}

TEST(RhythmAnalysisTest, FTX04_OnsetsAtBothHops)
{
    const std::vector<float> audio(4410);
    const auto onsets = measureOnsets(audio);

    EXPECT_EQ(onsets.beat.size(), 11U);
    EXPECT_EQ(onsets.detail.size(), 18U);
}

} // namespace
} // namespace snotes::rhythm
