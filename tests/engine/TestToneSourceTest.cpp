#include "engine/TestToneSource.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace snotes::engine
{
namespace
{

constexpr double kSampleRate = 48000.0;
constexpr double kFrequency = 1000.0;
constexpr float kGain = 0.25F;
constexpr float kTolerance = 1.0e-5F;

float expectedSample(int index)
{
    const double phase = kFrequency * static_cast<double>(index) / kSampleRate;
    return kGain * static_cast<float>(std::sin(2.0 * std::numbers::pi * phase));
}

class TestToneSourceTest : public ::testing::Test
{
protected:
    void SetUp() override { tone.prepare(kSampleRate); }

    std::vector<float> renderMono(int numSamples)
    {
        std::vector<float> buffer(static_cast<size_t>(numSamples), -1.0F);
        std::array<float*, 1> channels{buffer.data()};
        tone.render(channels.data(), 1, numSamples);
        return buffer;
    }

    TestToneSource tone{kFrequency, kGain};
};

TEST_F(TestToneSourceTest, FAO02_IsDisabledByDefault)
{
    EXPECT_FALSE(tone.isEnabled());
}

TEST_F(TestToneSourceTest, FAO02_ReportsEnabledStateAfterToggle)
{
    tone.setEnabled(true);
    EXPECT_TRUE(tone.isEnabled());
    tone.setEnabled(false);
    EXPECT_FALSE(tone.isEnabled());
}

TEST_F(TestToneSourceTest, FAO02_RendersSilenceWhileDisabled)
{
    for (const float sample : renderMono(64))
        EXPECT_FLOAT_EQ(sample, 0.0F);
}

TEST_F(TestToneSourceTest, FAO02_RendersSineAtConfiguredFrequencyAndGain)
{
    tone.setEnabled(true);
    const auto buffer = renderMono(100); // more than two periods, so the phase wraps

    for (int i = 0; i < 100; ++i)
        EXPECT_NEAR(buffer[static_cast<size_t>(i)], expectedSample(i), kTolerance) << "sample " << i;
}

TEST_F(TestToneSourceTest, FAO02_ContinuesPhaseAcrossBlocks)
{
    tone.setEnabled(true);
    const auto first = renderMono(30);
    const auto second = renderMono(30);

    EXPECT_NEAR(first[29], expectedSample(29), kTolerance);
    EXPECT_NEAR(second[0], expectedSample(30), kTolerance);
    EXPECT_NEAR(second[29], expectedSample(59), kTolerance);
}

TEST_F(TestToneSourceTest, FAO02_PrepareRestartsPhase)
{
    tone.setEnabled(true);
    (void)renderMono(13);
    tone.prepare(kSampleRate);

    EXPECT_NEAR(renderMono(2)[1], expectedSample(1), kTolerance);
}

TEST_F(TestToneSourceTest, FAO02_WritesSameSignalToEveryChannelAndSkipsInactiveOnes)
{
    tone.setEnabled(true);
    std::vector<float> left(8, -1.0F);
    std::vector<float> right(8, -1.0F);
    std::array<float*, 3> channels{left.data(), nullptr, right.data()};

    tone.render(channels.data(), 3, 8);

    for (size_t i = 0; i < 8; ++i)
    {
        EXPECT_NEAR(left[i], expectedSample(static_cast<int>(i)), kTolerance);
        EXPECT_FLOAT_EQ(right[i], left[i]);
    }
}

} // namespace
} // namespace snotes::engine
