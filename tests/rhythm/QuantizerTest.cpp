#include "rhythm/Quantizer.h"

#include "rhythm/RhythmReference.h"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace snotes::rhythm
{
namespace
{

std::string joined(const std::vector<int>& pitches)
{
    std::string text;
    for (const int pitch : pitches)
        text += (text.empty() ? "" : " ") + std::to_string(pitch);
    return text;
}

std::string joined(const std::vector<std::vector<int>>& graces)
{
    std::string text;
    for (std::size_t index = 0; index < graces.size(); ++index)
        text += (index == 0 ? "" : "|") + joined(graces[index]);
    return text;
}

/// The PoC pipeline from the notes to the quantized events, on the reference beats and onset strength.
class QuantizerReferenceTest : public ::testing::TestWithParam<std::string>
{
protected:
    [[nodiscard]] Quantization events() const
    {
        const auto notes = test::readNotes(GetParam());
        auto rules = rulesFor(guessStyle(notes).style);
        rules.dropGhostsUnderRinging = false;
        const auto beats = test::readBeats(GetParam(), true);
        return quantize(cleanUp(clusterOnsets(notes, rules.relativeAmplitude),
                                test::readFloats(GetParam() + "-oenv256.f32"),
                                beats,
                                rules),
                        beats);
    }

    [[nodiscard]] std::vector<std::string> summary() const
    {
        std::ifstream file{std::string{SNOTES_TEST_DATA_DIR} + "/rhythm/" + GetParam() + "-summary.txt"};
        std::vector<std::string> lines;
        for (std::string line; std::getline(file, line);)
            lines.push_back(line);
        return lines;
    }
};

TEST_P(QuantizerReferenceTest, E12_StyleMatchesThePoc)
{
    const auto guess = guessStyle(test::readNotes(GetParam()));
    const auto lines = summary();

    EXPECT_EQ(lines[0], guess.style == Style::riff ? "style riff" : "style melody");
    EXPECT_NEAR(guess.chordShare, std::stod(lines[1].substr(lines[1].find(' '))), 1e-6);
}

TEST_P(QuantizerReferenceTest, E12_EventsMatchThePoc)
{
    const auto expected = test::readTextRows(GetParam() + "-events.csv");
    const auto actual = events();

    ASSERT_EQ(actual.events.size(), expected.size());
    for (std::size_t index = 0; index < expected.size(); ++index)
    {
        const auto& event = actual.events[index];
        EXPECT_EQ(event.position.toString(), expected[index][0]) << index;
        EXPECT_EQ(event.duration.toString(), expected[index][1]) << index;
        EXPECT_NEAR(event.weight, std::stod(expected[index][2]), 1e-6) << index;
        EXPECT_EQ(joined(event.pitches), expected[index][3]) << index;
        EXPECT_EQ(joined(event.graces), expected[index][4]) << index;
    }
}

TEST_P(QuantizerReferenceTest, E12_GridUseMatchesThePoc)
{
    const auto use = events().gridUse;
    std::ostringstream text;
    text << "grid 1:" << use[0] << " 2:" << use[1] << " 4:" << use[2] << " 3:" << use[3];

    EXPECT_EQ(text.str(), summary()[2]);
}

INSTANTIATE_TEST_SUITE_P(Recordings, QuantizerReferenceTest, ::testing::Values("riff", "clean"));

TEST(FractionTest, FTX09_InLowestTerms)
{
    EXPECT_EQ(Fraction(2, 4).toString(), "1/2");
    EXPECT_EQ(Fraction(1, -2).toString(), "-1/2");
    EXPECT_EQ(Fraction(6, 3).toString(), "2");
    EXPECT_EQ(Fraction(1, 2) + Fraction(1, 3), Fraction(5, 6));
    EXPECT_EQ(Fraction(1, 2) - Fraction(1, 3), Fraction(1, 6));
    EXPECT_LT(Fraction(1, 3), Fraction(1, 2));
    EXPECT_NE(Fraction(1, 2), Fraction(1, 3));
    EXPECT_NE(Fraction(1, 2), Fraction(3, 2));
}

TEST(QuantizerTest, FTX09_BeatPositionsBetweenAndBeyondTheBeats)
{
    const std::vector<double> beats{1.0, 1.5, 2.5};

    EXPECT_DOUBLE_EQ(beatPosition(1.25, beats), 0.5);
    EXPECT_DOUBLE_EQ(beatPosition(1.5, beats), 1.0);
    EXPECT_DOUBLE_EQ(beatPosition(2.5, beats), 2.0);
    EXPECT_DOUBLE_EQ(beatPosition(0.75, beats), -0.5);
    EXPECT_DOUBLE_EQ(beatPosition(3.0, beats), 2.5);
}

Cluster clusterAt(double seconds, double endSeconds, double weight, int pitch)
{
    Cluster cluster;
    cluster.seconds = seconds;
    cluster.endSeconds = endSeconds;
    cluster.weight = weight;
    cluster.pitches = {pitch};
    return cluster;
}

const std::vector<double> kBeats{0.0, 1.0, 2.0, 3.0, 4.0, 5.0};

TEST(QuantizerTest, FTX09_TheHeavierEventWinsAPosition)
{
    // Three onsets within 40 ms land on the beat on every grid.
    const auto result = quantize(
        {clusterAt(1.0, 1.2, 0.5, 40), clusterAt(1.02, 1.2, 0.9, 41), clusterAt(1.04, 1.2, 0.1, 42)}, kBeats);

    ASSERT_EQ(result.events.size(), 1U);
    EXPECT_EQ(result.events[0].pitches, std::vector<int>{41});
    EXPECT_EQ(result.gridUse[0], 1);
}

TEST(QuantizerTest, FTX09_GapsOverHalfABeatStay)
{
    const auto result = quantize({clusterAt(0.0, 0.5, 1.0, 40), clusterAt(3.0, 3.5, 1.0, 41)}, kBeats);

    ASSERT_EQ(result.events.size(), 2U);
    EXPECT_EQ(result.events[0].duration, Fraction(1, 2));
    EXPECT_EQ(result.events[1].duration, Fraction(1, 2));
}

TEST(QuantizerTest, FTX09_ResultsDifferInEveryField)
{
    const QuantizedEvent event{
        .position = {1}, .duration = {1, 2}, .seconds = 1.0, .weight = 1.0, .pitches = {40}, .graces = {}};
    auto other = event;
    EXPECT_EQ(other, event);
    other.position = {2};
    EXPECT_NE(other, event);
    other = event;
    other.duration = {1};
    EXPECT_NE(other, event);
    other = event;
    other.seconds = 2.0;
    EXPECT_NE(other, event);
    other = event;
    other.weight = 2.0;
    EXPECT_NE(other, event);
    other = event;
    other.pitches = {41};
    EXPECT_NE(other, event);
    other = event;
    other.graces = {{39}};
    EXPECT_NE(other, event);
    const Quantization quantization{.events = {event}, .gridUse = {1, 0, 0, 0}};
    EXPECT_NE(quantization, (Quantization{.events = {}, .gridUse = {1, 0, 0, 0}}));
    EXPECT_NE(quantization, (Quantization{.events = {event}, .gridUse = {0, 1, 0, 0}}));
}

} // namespace
} // namespace snotes::rhythm
