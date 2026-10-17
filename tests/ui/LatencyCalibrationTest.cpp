#include "ui/LatencyCalibration.h"

#include "io/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <vector>

namespace snotes::ui
{
namespace
{

using ::testing::_;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;

class LatencyCalibrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, writeText(_, _)).WillByDefault(Return(true));
        info.open = true;
        info.sampleRate = 1000.0; // probe window of 500 samples
        info.bufferSize = 16;
        info.inputLatencySamples = 2;
        info.outputLatencySamples = 3;
        probe.prepare(info.sampleRate);
    }

    /// Runs the audio thread for `numSamples` with output 0 looped into input 0 after `delay` samples.
    void loopback(int delay, int numSamples)
    {
        std::vector<float> cable(static_cast<std::size_t>(delay), 0.0F);
        constexpr int kBlock = 4;
        std::array<float, kBlock> input{};
        std::array<float, kBlock> output{};
        for (int done = 0; done < numSamples; done += kBlock)
        {
            std::copy(cable.begin(), cable.begin() + kBlock, input.begin());
            cable.erase(cable.begin(), cable.begin() + kBlock);
            output.fill(0.0F);
            const std::array<const float*, 1> inputs{input.data()};
            std::array<float*, 1> outputs{output.data()};
            probe.process(inputs.data(), 1, outputs.data(), 1, kBlock);
            cable.insert(cable.end(), output.begin(), output.end());
        }
    }

    NiceMock<io::MockFileSystem> fileSystem;
    io::SettingsStore store{fileSystem, "latency-offset.txt"};
    LatencyCompensation compensation{store};
    engine::LatencyProbe probe;
    engine::Transport transport;
    AudioDeviceInfo info;
    LatencyCalibration calibration{probe, transport, compensation, [this] { return info; }};
};

TEST_F(LatencyCalibrationTest, Q05_MeasurementBecomesTheCompensation)
{
    transport.play();
    calibration.start(0);
    EXPECT_FALSE(transport.isPlaying());
    EXPECT_TRUE(calibration.isRunning());
    EXPECT_EQ(calibration.poll(), std::nullopt);

    loopback(12, 3000);

    const auto result = calibration.poll();
    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(*result, HasSubstr("12,0 ms"));
    EXPECT_EQ(compensation.samples(info), 12);
    EXPECT_EQ(calibration.poll(), std::nullopt);
}

TEST_F(LatencyCalibrationTest, Q01_FailedMeasurementKeepsTheCompensation)
{
    calibration.start(3);

    loopback(12, 3000);

    EXPECT_THAT(calibration.poll().value_or(""), HasSubstr("Kabel"));
    EXPECT_DOUBLE_EQ(compensation.offsetMs(), 0.0);
}

TEST_F(LatencyCalibrationTest, Q01_CancelledMeasurementReportsNothing)
{
    calibration.start(0);
    loopback(12, 100);

    calibration.cancel();

    EXPECT_FALSE(calibration.isRunning());
    EXPECT_EQ(calibration.poll(), std::nullopt);
}

TEST_F(LatencyCalibrationTest, FWI05_DescribesTheCompensation)
{
    compensation.setOffsetMs(-1.0);

    EXPECT_EQ(calibration.compensationText(),
              "Neue Takes werden um 4,0 ms verschoben (Treiber 5,0 ms, Versatz -1,0 ms).");
}

TEST_F(LatencyCalibrationTest, FWI05_NoDeviceNoCompensation)
{
    info.open = false;

    EXPECT_THAT(calibration.compensationText(), HasSubstr("Kein Audiogerät"));
}

} // namespace
} // namespace snotes::ui
