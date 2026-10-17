#include "engine/InputMeter.h"

#include <gtest/gtest.h>

#include <array>
#include <vector>

namespace snotes::engine
{
namespace
{

TEST(InputMeterTest, FAU03_KeepsTheHighestPeakUntilTaken)
{
    InputMeter meter;
    std::array<float, 3> loud{0.1F, -0.8F, 0.3F};
    std::array<float, 3> quiet{0.2F, 0.0F, 0.0F};
    const std::array<const float*, 1> loudBlock{loud.data()};
    const std::array<const float*, 1> quietBlock{quiet.data()};

    meter.process(loudBlock.data(), 1, 3);
    meter.process(quietBlock.data(), 1, 3);

    EXPECT_FLOAT_EQ(meter.takePeak(0), 0.8F);
    EXPECT_FLOAT_EQ(meter.takePeak(0), 0.0F);
}

TEST(InputMeterTest, FAU03_SkipsInactiveChannelsAndUnknownInputs)
{
    InputMeter meter;
    std::array<float, 1> signal{0.5F};
    const std::array<const float*, 2> channels{nullptr, signal.data()};

    meter.process(channels.data(), 2, 1);

    EXPECT_FLOAT_EQ(meter.takePeak(0), 0.0F);
    EXPECT_FLOAT_EQ(meter.takePeak(1), 0.5F);
    EXPECT_FLOAT_EQ(meter.takePeak(-1), 0.0F);
    EXPECT_FLOAT_EQ(meter.takePeak(InputMeter::kMaxInputs), 0.0F);
}

TEST(InputMeterTest, FAU03_IgnoresChannelsBeyondTheMaximum)
{
    InputMeter meter;
    std::array<float, 1> signal{0.5F};
    const std::vector<const float*> channels(InputMeter::kMaxInputs + 2, signal.data());

    meter.process(channels.data(), static_cast<int>(channels.size()), 1);

    EXPECT_FLOAT_EQ(meter.takePeak(InputMeter::kMaxInputs - 1), 0.5F);
}

} // namespace
} // namespace snotes::engine
