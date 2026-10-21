#include "rhythm/OnsetStrength.h"

#include "rhythm/RhythmReference.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace snotes::rhythm
{
namespace
{

/// Largest difference relative to the largest value of the reference.
double relativeError(const std::vector<float>& actual, const std::vector<float>& expected)
{
    double error = 0.0;
    for (std::size_t index = 0; index < expected.size(); ++index)
        error = std::max(error, static_cast<double>(std::abs(actual[index] - expected[index])));
    return error / *std::ranges::max_element(expected);
}

TEST(OnsetStrengthTest, E12_MatchesLibrosaAtBothHops)
{
    const auto audio = test::readFloats("riff-audio-22050.f32");
    for (const int hop : {kDetailHop, kBeatHop})
    {
        const auto expected = test::readFloats("riff-audio-oenv" + std::to_string(hop) + ".f32");
        const auto actual = onsetStrength(audio, hop);

        ASSERT_EQ(actual.size(), expected.size());
        EXPECT_LT(relativeError(actual, expected), 1e-4) << hop;
    }
}

TEST(OnsetStrengthTest, FTX04_RisesAtAnAttack)
{
    std::vector<float> audio(22050);
    for (std::size_t index = 11025; index < audio.size(); ++index)
        audio[index] = static_cast<float>(
            0.5 * std::sin(2.0 * std::numbers::pi * 440.0 * static_cast<double>(index) / kAnalysisRate));
    const auto envelope = onsetStrength(audio, kDetailHop);

    const auto peak = std::ranges::max_element(envelope) - envelope.begin();
    EXPECT_NEAR(frameSeconds(static_cast<std::size_t>(peak), kDetailHop), 0.5, 0.03);
    EXPECT_EQ(envelope.front(), 0.0F);
}

TEST(OnsetStrengthTest, FTX04_SilenceHasNoOnsets)
{
    const std::vector<float> audio(1000);

    EXPECT_EQ(onsetStrength(audio, kBeatHop), std::vector<float>(3));
    EXPECT_EQ(onsetStrength({}, kBeatHop), std::vector<float>(1));
}

TEST(OnsetStrengthTest, FTX04_FramesStartAtZero)
{
    EXPECT_DOUBLE_EQ(frameSeconds(0, kBeatHop), 0.0);
    EXPECT_DOUBLE_EQ(frameSeconds(50, kBeatHop), 1.0);
}

} // namespace
} // namespace snotes::rhythm
