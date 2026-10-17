#include "engine/LatencyProbe.h"

#include <gtest/gtest.h>

#include <array>
#include <deque>
#include <vector>

namespace snotes::engine
{
namespace
{

constexpr double kRate = 1000.0; // window of 500 samples

/// Output 0 runs through a cable of `delay` samples into input 1; the delay is at least one block, as on
/// a real device.
class Loopback
{
public:
    explicit Loopback(int delay, float gain = 1.0F)
        : cable_(static_cast<std::size_t>(delay), 0.0F), gain_(gain)
    {
    }

    void run(LatencyProbe& probe, int numBlocks, int blockSize = 16)
    {
        std::vector<float> silent(static_cast<std::size_t>(blockSize));
        std::vector<float> input(static_cast<std::size_t>(blockSize));
        std::vector<float> output(static_cast<std::size_t>(blockSize));
        for (int block = 0; block < numBlocks; ++block)
        {
            for (auto& sample : input)
            {
                sample = gain_ * cable_.front();
                cable_.pop_front();
            }
            std::fill(output.begin(), output.end(), 0.0F);
            const std::array<const float*, 2> inputs{silent.data(), input.data()};
            std::array<float*, 2> outputs{output.data(), nullptr};
            probe.process(inputs.data(), 2, outputs.data(), 2, blockSize);
            cable_.insert(cable_.end(), output.begin(), output.end());
        }
    }

private:
    std::deque<float> cable_;
    float gain_;
};

class LatencyProbeTest : public ::testing::Test
{
protected:
    void SetUp() override { probe.prepare(kRate); }

    LatencyProbe probe;
};

TEST_F(LatencyProbeTest, Q01_MeasuresTheRoundTripOfEveryPing)
{
    Loopback loopback{37};
    probe.start(1);
    EXPECT_TRUE(probe.isRunning());

    loopback.run(probe, 200);

    EXPECT_FALSE(probe.isRunning());
    ASSERT_EQ(probe.numFinished(), LatencyProbe::kPings);
    for (int ping = 0; ping < LatencyProbe::kPings; ++ping)
        EXPECT_EQ(probe.result(ping), 37);
}

TEST_F(LatencyProbeTest, Q01_NothingArrivedWithoutCable)
{
    Loopback loopback{37, 0.0F};
    probe.start(1);

    loopback.run(probe, 200);

    EXPECT_EQ(probe.result(0), LatencyProbe::kNoPeak);
}

TEST_F(LatencyProbeTest, Q01_InputTheDeviceDoesNotHaveHearsNothing)
{
    Loopback loopback{37};
    probe.start(5);

    loopback.run(probe, 200);

    EXPECT_EQ(probe.numFinished(), LatencyProbe::kPings);
    EXPECT_EQ(probe.result(0), LatencyProbe::kNoPeak);
}

TEST_F(LatencyProbeTest, Q01_NoInputChosenHearsNothing)
{
    Loopback loopback{37};
    probe.start(-1);

    loopback.run(probe, 200);

    EXPECT_EQ(probe.numFinished(), LatencyProbe::kPings);
    EXPECT_EQ(probe.result(0), LatencyProbe::kNoPeak);
}

TEST_F(LatencyProbeTest, Q01_IdleProbeLeavesTheOutputsAlone)
{
    Loopback loopback{37};

    loopback.run(probe, 10);

    EXPECT_EQ(probe.numFinished(), 0);
}

TEST_F(LatencyProbeTest, Q01_CancelStopsThePings)
{
    Loopback loopback{37};
    probe.start(1);
    loopback.run(probe, 35); // first ping finished after 500 samples

    probe.cancel();
    loopback.run(probe, 200);

    EXPECT_FALSE(probe.isRunning());
    EXPECT_EQ(probe.numFinished(), 1);
}

TEST_F(LatencyProbeTest, Q01_RestartBeginsAFreshMeasurement)
{
    Loopback loopback{37};
    probe.start(1);
    loopback.run(probe, 3);

    probe.start(1);
    loopback.run(probe, 200);

    EXPECT_EQ(probe.result(0), 37);
    EXPECT_EQ(probe.numFinished(), LatencyProbe::kPings);
}

} // namespace
} // namespace snotes::engine
