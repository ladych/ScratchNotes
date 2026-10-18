#include "ui/LatencyCompensation.h"

#include "io/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace snotes::ui
{
namespace
{

using ::testing::_;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;

const std::filesystem::path kFile{"config/latency-offset.txt"};

AudioDeviceInfo device()
{
    AudioDeviceInfo info;
    info.open = true;
    info.sampleRate = 48000.0;
    info.bufferSize = 128;
    info.inputLatencySamples = 144;  // 3 ms
    info.outputLatencySamples = 240; // 5 ms
    return info;
}

class LatencyCompensationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, writeText(_, _)).WillByDefault(Return(true));
    }

    NiceMock<io::MockFileSystem> fileSystem;
    io::SettingsStore store{fileSystem, kFile};
};

TEST_F(LatencyCompensationTest, FWI05_CompensatesTheReportedRoundTrip)
{
    const LatencyCompensation compensation{store};

    EXPECT_DOUBLE_EQ(compensation.offsetMs(), 0.0);
    EXPECT_EQ(compensation.samples(device()), 384);
}

TEST_F(LatencyCompensationTest, FWI05_AddsTheOffsetAndKeepsIt)
{
    LatencyCompensation compensation{store};

    EXPECT_CALL(fileSystem, writeText(kFile, "1500")).WillOnce(Return(true));
    EXPECT_TRUE(compensation.setOffsetMs(1.5));

    EXPECT_EQ(compensation.samples(device()), 384 + 72);
}

TEST_F(LatencyCompensationTest, FWI05_RestoresTheSavedOffset)
{
    EXPECT_CALL(fileSystem, readText(kFile)).WillOnce(Return(std::string{"-2000"}));

    const LatencyCompensation compensation{store};

    EXPECT_DOUBLE_EQ(compensation.offsetMs(), -2.0);
}

TEST_F(LatencyCompensationTest, FWI05_IgnoresAGarbledSetting)
{
    EXPECT_CALL(fileSystem, readText(kFile)).WillOnce(Return(std::string{"viel"}));

    const LatencyCompensation compensation{store};

    EXPECT_DOUBLE_EQ(compensation.offsetMs(), 0.0);
}

TEST_F(LatencyCompensationTest, FWI05_NeverMovesTakesIntoTheFuture)
{
    LatencyCompensation compensation{store};
    compensation.setOffsetMs(-50.0);

    EXPECT_EQ(compensation.samples(device()), 0);
}

TEST_F(LatencyCompensationTest, Q05_CalibrationMakesTheCompensationEqualTheMeasurement)
{
    LatencyCompensation compensation{store};

    EXPECT_TRUE(compensation.calibrate(500, device()));

    EXPECT_EQ(compensation.samples(device()), 500);
}

TEST_F(LatencyCompensationTest, FWI05_FailedSaveIsReported)
{
    LatencyCompensation compensation{store};
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(Return(false));

    EXPECT_FALSE(compensation.setOffsetMs(1.0));
    EXPECT_DOUBLE_EQ(compensation.offsetMs(), 1.0);
}

TEST(MeasuredRoundTripTest, Q01_TakesTheMedianOfThePingsThatArrived)
{
    EXPECT_EQ(measuredRoundTrip({500, -1, 502, 501, 499}, 48000.0), 501);
}

TEST(MeasuredRoundTripTest, Q01_TooFewPingsGiveNoResult)
{
    EXPECT_EQ(measuredRoundTrip({500, -1, 502, -1, -1}, 48000.0), std::nullopt);
}

TEST(MeasuredRoundTripTest, Q01_ScatteredPingsGiveNoResult)
{
    EXPECT_EQ(measuredRoundTrip({500, 501, 600}, 48000.0), std::nullopt);
}

TEST(MeasurementTextTest, Q01_ShowsMeasuredAndReportedRoundTrip)
{
    EXPECT_EQ(
        measurementText(480, device()),
        "Gemessener Roundtrip 10,0 ms, der Treiber meldet 8,0 ms. Neue Takes werden um 10,0 ms verschoben.");
}

TEST(MeasurementTextTest, Q01_ExplainsAFailedMeasurement)
{
    EXPECT_THAT(measurementText(std::nullopt, device()), HasSubstr("Kabel"));
}

} // namespace
} // namespace snotes::ui
