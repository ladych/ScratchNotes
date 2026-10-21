#include "rhythm/BeatTracker.h"

#include "rhythm/OnsetStrength.h"
#include "rhythm/RhythmReference.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cmath>
#include <numeric>

#include <string>
#include <vector>

namespace snotes::rhythm
{
namespace
{

constexpr double kBeatRate = kAnalysisRate / kBeatHop;

class BeatTrackerReferenceTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(BeatTrackerReferenceTest, E12_TempoCandidatesMatchThePoc)
{
    const auto expected = test::readRows(GetParam() + "-tempo.csv");
    const auto actual = tempoCandidates(test::readFloats(GetParam() + "-oenv441.f32"), kBeatRate);

    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        EXPECT_NEAR(actual[index].bpm, expected[index][0], 1e-4);
        EXPECT_NEAR(actual[index].share, expected[index][1], 1e-4);
    }
}

TEST_P(BeatTrackerReferenceTest, E12_BeatsMatchThePoc)
{
    const auto envelope = test::readFloats(GetParam() + "-oenv441.f32");
    const auto expected = test::readBeats(GetParam(), false);
    const auto actual =
        trackBeats(envelope, kBeatRate, tempoCandidates(envelope, kBeatRate)[0].bpm, kLibrosaWindow);

    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index)
        EXPECT_NEAR(actual[index], expected[index], 1e-6) << index;
}

INSTANTIATE_TEST_SUITE_P(Recordings, BeatTrackerReferenceTest, ::testing::Values("riff", "clean"));

TEST(BeatTrackerTest, FTX04_NoTempoWithoutOnsets)
{
    EXPECT_TRUE(tempoCandidates({}, kBeatRate).empty());
    EXPECT_TRUE(tempoCandidates(std::vector<float>(10, 1.0F), kBeatRate).empty());
}

TEST(BeatTrackerTest, FTX04_CandidatesDifferByMoreThanFourPercent)
{
    // Noise has many peaks in its autocorrelation, at this frame rate only a few lags apart.
    std::vector<float> noise(3000);
    unsigned state = 1;
    for (auto& value : noise)
    {
        state = (state * 1103515245U) + 12345U;
        value = static_cast<float>((state >> 16U) % 1000U) / 1000.0F;
    }
    const auto candidates = tempoCandidates(noise, 1000.0);

    ASSERT_EQ(candidates.size(), 4U);
    for (std::size_t a = 0; a < candidates.size(); ++a)
        for (std::size_t b = 0; b < a; ++b)
            EXPECT_GE(std::abs(candidates[a].bpm - candidates[b].bpm) / candidates[b].bpm, 0.04);
    const double total = std::accumulate(candidates.begin(),
                                         candidates.end(),
                                         0.0,
                                         [](double sum, const TempoCandidate& c) { return sum + c.share; });
    EXPECT_NEAR(total, 1.0, 1e-9);
}

TEST(BeatTrackerTest, FTX04_SkipsATempoCloseToABetterOne)
{
    // Beats alternately 495 and 505 frames apart: two peaks 2 % apart, only the better one counts.
    std::vector<float> envelope(20000);
    for (std::size_t frame = 100, beat = 0; frame < envelope.size(); frame += beat++ % 2 == 0 ? 495 : 505)
        envelope[frame] = 1.0F;

    const auto candidates = tempoCandidates(envelope, 1000.0);

    ASSERT_FALSE(candidates.empty());
    for (std::size_t a = 0; a < candidates.size(); ++a)
        for (std::size_t b = 0; b < a; ++b)
            EXPECT_GE(std::abs(candidates[a].bpm - candidates[b].bpm) / candidates[b].bpm, 0.04);
}

TEST(BeatTrackerTest, FTX04_CandidatesDifferInEveryField)
{
    const TempoCandidate candidate{.bpm = 100.0, .share = 0.5};

    EXPECT_NE(candidate, (TempoCandidate{.bpm = 90.0, .share = 0.5}));
    EXPECT_NE(candidate, (TempoCandidate{.bpm = 100.0, .share = 0.4}));
}

TEST(BeatTrackerTest, FTX04_NoBeatsInSilence)
{
    EXPECT_TRUE(trackBeats(std::vector<float>(100), kBeatRate, 100.0, BeatTrackerSettings{}).empty());
    EXPECT_TRUE(trackBeats(std::vector<float>{1.0F}, kBeatRate, 100.0, BeatTrackerSettings{}).empty());
}

TEST(BeatTrackerTest, FTX04_LastBeatIsTheLastStrongPeak)
{
    EXPECT_EQ(lastBeat({0.0, 10.0, 0.0, 1.0, 0.0}), 1);
    EXPECT_EQ(lastBeat({0.0, 1.0, 2.0}), 2);
    // Without any peak, or none above half their median, the last frame.
    EXPECT_EQ(lastBeat({5.0, 4.0, 3.0, 2.0}), 3);
    EXPECT_EQ(lastBeat({-10.0, -1.0, -10.0, -1.0, -5.0}), 4);
}

TEST(BeatTrackerTest, FTX04_TrimClearsWeakFramesAtBothEnds)
{
    std::vector<bool> beats{true, true, true};
    trimBeats({1.0, 1.0, 1.0}, beats);
    EXPECT_THAT(beats, ::testing::ElementsAre(true, true, true));

    beats = {false, true, false, true, false};
    trimBeats({0.0, 0.0, 0.0, 0.0, 0.0}, beats);
    EXPECT_THAT(beats, ::testing::Each(false));
}

} // namespace
} // namespace snotes::rhythm
