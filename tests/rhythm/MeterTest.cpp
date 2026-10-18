#include "rhythm/Meter.h"

#include "rhythm/RhythmReference.h"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

namespace snotes::rhythm
{
namespace
{

class MeterReferenceTest : public ::testing::TestWithParam<std::string>
{
protected:
    [[nodiscard]] BeatFeatures features() const
    {
        return beatFeatures(test::readBeats(GetParam(), true),
                            test::readNotes(GetParam()),
                            test::readFloats(GetParam() + "-oenv256.f32"));
    }
};

TEST_P(MeterReferenceTest, E12_BeatFeaturesMatchThePoc)
{
    const auto expected = test::readRows(GetParam() + "-features.csv");
    const auto actual = features();

    ASSERT_EQ(actual.chordChange.size(), expected.size());
    for (std::size_t beat = 0; beat < expected.size(); ++beat)
    {
        EXPECT_NEAR(actual.chordChange[beat], expected[beat][0], 1e-6) << beat;
        EXPECT_NEAR(actual.onset[beat], expected[beat][1], 1e-6) << beat;
        EXPECT_NEAR(actual.durational[beat], expected[beat][2], 1e-6) << beat;
    }
}

TEST_P(MeterReferenceTest, E12_MeterRankingMatchesThePoc)
{
    const auto rows = test::readTextRows(GetParam() + "-meter.csv");
    const auto actual = estimateMeter(features());

    ASSERT_EQ(actual.ranking.size(), 5U);
    for (std::size_t index = 0; index < actual.ranking.size(); ++index)
    {
        EXPECT_EQ(actual.ranking[index].beatsPerBar, std::stoi(rows[index][0]));
        EXPECT_NEAR(actual.ranking[index].confidence, std::stod(rows[index][1]), 1e-6);
        EXPECT_EQ(actual.ranking[index].phase, std::stoi(rows[index][2]));
    }
    EXPECT_EQ(actual.phrase.beats, std::stoi(rows[5][1]));
    EXPECT_NEAR(actual.phrase.similarity, std::stod(rows[5][2]), 1e-6);
    EXPECT_EQ(actual.phrase.strong, rows[5][3] == "1");
}

INSTANTIATE_TEST_SUITE_P(Recordings, MeterReferenceTest, ::testing::Values("riff", "clean"));

std::array<double, 12> pitchClass(std::size_t index)
{
    std::array<double, 12> chroma{};
    chroma.at(index) = 1.0;
    return chroma;
}

TEST(MeterTest, FTX06_NoPhraseInFewerThanEightBeats)
{
    EXPECT_EQ(phrasePeriod(std::vector<std::array<double, 12>>(7, pitchClass(0))), Phrase{});
}

TEST(MeterTest, FTX06_TheShortestRepeatingPhrase)
{
    // C C C G, eight times: the harmony repeats every four beats.
    std::vector<std::array<double, 12>> chroma;
    for (int beat = 0; beat < 32; ++beat)
        chroma.push_back(pitchClass(beat % 4 == 3 ? 7 : 0));

    const auto phrase = phrasePeriod(chroma);

    EXPECT_EQ(phrase.beats, 4);
    EXPECT_DOUBLE_EQ(phrase.similarity, 1.0);
    EXPECT_TRUE(phrase.strong);
    // The same chord all the time repeats, but not more than everything else.
    EXPECT_FALSE(phrasePeriod(std::vector<std::array<double, 12>>(16, pitchClass(0))).strong);
}

TEST(MeterTest, FTX06_FewBeatsStillGiveAllMeters)
{
    BeatFeatures features;
    features.chordChange = {1.0, 0.0, 0.0, 1.0};
    features.onset = {1.0, 0.5, 0.2, 1.0};
    features.durational = {0.5, 0.0, 0.0, 0.5};
    features.chroma = std::vector<std::array<double, 12>>(4, pitchClass(0));

    const auto estimate = estimateMeter(features);

    EXPECT_EQ(estimate.ranking.size(), 5U);
    EXPECT_EQ(estimate.ranking.front().beatsPerBar, 3);
}

TEST(MeterTest, FTX06_ResultsDifferInEveryField)
{
    const Phrase phrase{.beats = 4, .similarity = 0.5, .strong = true};
    EXPECT_NE(phrase, (Phrase{.beats = 8, .similarity = 0.5, .strong = true}));
    EXPECT_NE(phrase, (Phrase{.beats = 4, .similarity = 0.6, .strong = true}));
    EXPECT_NE(phrase, (Phrase{.beats = 4, .similarity = 0.5, .strong = false}));
    const MeterCandidate candidate{.beatsPerBar = 4, .confidence = 0.5, .phase = 1};
    EXPECT_NE(candidate, (MeterCandidate{.beatsPerBar = 3, .confidence = 0.5, .phase = 1}));
    EXPECT_NE(candidate, (MeterCandidate{.beatsPerBar = 4, .confidence = 0.6, .phase = 1}));
    EXPECT_NE(candidate, (MeterCandidate{.beatsPerBar = 4, .confidence = 0.5, .phase = 0}));
    const MeterEstimate estimate{.ranking = {candidate}, .phrase = phrase};
    EXPECT_NE(estimate, (MeterEstimate{.ranking = {}, .phrase = phrase}));
    EXPECT_NE(estimate, (MeterEstimate{.ranking = {candidate}, .phrase = {}}));
}

} // namespace
} // namespace snotes::rhythm
