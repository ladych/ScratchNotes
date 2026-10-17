#include "ui/LevelMeter.h"
#include "ui/RecordingTime.h"

#include <gtest/gtest.h>

namespace snotes::ui
{
namespace
{

TEST(LevelMeterTest, FAU03_StartsSilent)
{
    const LevelMeter meter;

    EXPECT_FLOAT_EQ(meter.position(), 0.0F);
    EXPECT_FALSE(meter.clipped());
}

TEST(LevelMeterTest, FAU03_ShowsThePeakInDecibels)
{
    LevelMeter meter;

    meter.update(0.1F, 0.0); // -20 dB
    EXPECT_NEAR(meter.position(), 40.0F / 60.0F, 1e-5F);

    meter.update(1.0F, 0.0);
    EXPECT_FLOAT_EQ(meter.position(), 1.0F);
}

TEST(LevelMeterTest, FAU03_FallsSlowlyAfterAPeak)
{
    LevelMeter meter;
    meter.update(1.0F, 0.0);

    meter.update(0.0F, 0.5); // falls 12 dB

    EXPECT_NEAR(meter.position(), 48.0F / 60.0F, 1e-5F);

    meter.update(0.0F, 10.0);
    EXPECT_FLOAT_EQ(meter.position(), 0.0F);
}

TEST(LevelMeterTest, FAU03_ClipStaysUntilReset)
{
    LevelMeter meter;

    meter.update(1.2F, 0.0);
    meter.update(0.1F, 5.0);
    EXPECT_TRUE(meter.clipped());

    meter.resetClip();
    EXPECT_FALSE(meter.clipped());
    meter.update(0.998F, 0.0);
    EXPECT_FALSE(meter.clipped());
}

TEST(RecordingTimeTest, FAU02_ShowsMinutesSecondsAndTenths)
{
    EXPECT_EQ(formatRecordingTime(0.0), "0:00,0");
    EXPECT_EQ(formatRecordingTime(65.37), "1:05,3");
    EXPECT_EQ(formatRecordingTime(600.0), "10:00,0");
    EXPECT_EQ(formatRecordingTime(12.0), "0:12,0");
}

} // namespace
} // namespace snotes::ui
