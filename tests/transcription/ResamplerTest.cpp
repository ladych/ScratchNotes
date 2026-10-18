#include "transcription/Resampler.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace snotes::transcription
{
namespace
{

std::vector<float> sine(double hz, double rate, double seconds)
{
    std::vector<float> samples(static_cast<std::size_t>(rate * seconds));
    for (std::size_t index = 0; index < samples.size(); ++index)
        samples[index] = static_cast<float>(
            0.5 * std::sin(2.0 * std::numbers::pi * hz * static_cast<double>(index) / rate));
    return samples;
}

/// Peak of the middle half, away from the edges where the filter sees silence.
float middlePeak(const std::vector<float>& samples)
{
    const auto quarter = static_cast<std::ptrdiff_t>(samples.size() / 4);
    float peak = 0.0F;
    std::for_each(samples.begin() + quarter,
                  samples.end() - quarter,
                  [&peak](float value) { peak = std::max(peak, std::abs(value)); });
    return peak;
}

TEST(ResamplerTest, FTX02_SameRateKeepsTheSamples)
{
    const std::vector<float> samples{0.1F, -0.2F, 0.3F};

    EXPECT_EQ(resample(samples, 48000.0, 48000.0), samples);
}

TEST(ResamplerTest, FTX02_LengthFollowsTheRatioRoundedUp)
{
    const std::vector<float> samples(1001);

    EXPECT_EQ(resample(samples, 44100.0, 22050.0).size(), 501U);
    EXPECT_EQ(resample(samples, 192000.0, 22050.0).size(), 115U);
    EXPECT_TRUE(resample({}, 48000.0, 22050.0).empty());
}

TEST(ResamplerTest, FTX02_KeepsTonesBelowTheNewNyquistFrequency)
{
    const auto down = resample(sine(440.0, 192000.0, 1.0), 192000.0, 22050.0);
    const auto up = resample(sine(440.0, 11025.0, 1.0), 11025.0, 22050.0);

    EXPECT_NEAR(middlePeak(down), 0.5F, 0.005F);
    EXPECT_NEAR(middlePeak(up), 0.5F, 0.005F);
    const auto expected = sine(440.0, 22050.0, 1.0);
    for (std::size_t index = 5000; index < 6000; ++index)
        EXPECT_NEAR(down[index], expected[index], 0.005F);
}

TEST(ResamplerTest, FTX02_RemovesTonesAboveTheNewNyquistFrequency)
{
    const auto result = resample(sine(15000.0, 48000.0, 1.0), 48000.0, 22050.0);

    EXPECT_LT(middlePeak(result), 0.5F * 0.001F);
}

} // namespace
} // namespace snotes::transcription
