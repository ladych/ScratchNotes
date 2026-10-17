#include "engine/ClickTrack.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace snotes::engine
{
namespace
{

constexpr double kRate = 48000.0;
constexpr std::int64_t kClickLength = 1440; // 30 ms
constexpr std::int64_t kBeat = 24000;       // 120 bpm

class ClickTrackTest : public ::testing::Test
{
protected:
    void SetUp() override { click.prepare(kRate); }

    /// Renders `numSamples` samples from `position` on into two outputs, the second one inactive.
    std::vector<float> render(std::int64_t position, int numSamples, double bpm = 120.0)
    {
        std::vector<float> samples(static_cast<std::size_t>(numSamples));
        std::array<float*, 2> outputs{samples.data(), nullptr};
        click.render(outputs.data(), 2, numSamples, position, bpm);
        return samples;
    }

    ClickTrack click;
};

float peak(const std::vector<float>& samples)
{
    float result = 0.0F;
    for (const float value : samples)
        result = std::max(result, std::abs(value));
    return result;
}

TEST_F(ClickTrackTest, FWI07_ClicksOnEveryBeat)
{
    EXPECT_GT(peak(render(0, 100)), 0.0F);
    EXPECT_EQ(peak(render(kClickLength, 100)), 0.0F);
    EXPECT_EQ(peak(render(kBeat - 100, 100)), 0.0F);
    EXPECT_GT(peak(render(kBeat, 100)), 0.0F);
}

TEST_F(ClickTrackTest, FWI07_StaysOnTheBeatAfterALocate)
{
    const auto whole = render(kBeat - 10, 50);
    const auto part = render(kBeat + 5, 20);

    for (std::size_t index = 0; index < part.size(); ++index)
        EXPECT_FLOAT_EQ(part[index], whole[index + 15]);
}

TEST_F(ClickTrackTest, FWI07_FirstBeatOfTheBarIsHigher)
{
    EXPECT_GT(click.sampleAt(1, true), click.sampleAt(1, false));
    EXPECT_EQ(render(0, 2)[1], click.sampleAt(1, true));
    EXPECT_EQ(render(kBeat, 2)[1], click.sampleAt(1, false));
    EXPECT_EQ(render(4 * kBeat, 2)[1], click.sampleAt(1, true));
}

TEST_F(ClickTrackTest, FWI07_ClickFadesOut)
{
    EXPECT_EQ(click.sampleAt(-1, false), 0.0F);
    EXPECT_EQ(click.sampleAt(kClickLength, false), 0.0F);
    EXPECT_LT(std::abs(click.sampleAt(kClickLength - 2, false)), 0.01F);
}

TEST_F(ClickTrackTest, FWI07_AddsToTheOutputs)
{
    std::array<float, 1> samples{1.0F};
    std::array<float*, 1> outputs{samples.data()};

    click.render(outputs.data(), 1, 1, kClickLength, 120.0);

    EXPECT_FLOAT_EQ(samples[0], 1.0F);
}

TEST_F(ClickTrackTest, FWI07_NoTempoNoClick)
{
    EXPECT_EQ(peak(render(0, 100, 0.0)), 0.0F);
}

} // namespace
} // namespace snotes::engine
