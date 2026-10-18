#include "ui/TransportControl.h"

#include <gtest/gtest.h>

namespace snotes::ui
{
namespace
{

class TransportControlTest : public ::testing::Test
{
protected:
    /// Lets the audio thread apply the requests and play `numSamples` samples.
    void audioBlock(int numSamples) { transport.endBlock(transport.beginBlock(), numSamples); }

    engine::Transport transport;
    double sampleRate = 48000.0;
    bool recording = false;
    int recordingStops = 0;
    TransportControl control{transport,
                             {.sampleRate = [this] { return sampleRate; },
                              .isRecording = [this] { return recording; },
                              .stopRecording = [this] { ++recordingStops; }}};
};

TEST_F(TransportControlTest, FWI01_PlayAndStopToggle)
{
    control.togglePlay();
    EXPECT_TRUE(control.isPlaying());

    control.togglePlay();
    EXPECT_FALSE(control.isPlaying());
}

TEST_F(TransportControlTest, FWI01_StopWhileRecordingEndsTheRecording)
{
    recording = true;

    control.togglePlay();

    EXPECT_EQ(recordingStops, 1);
    EXPECT_FALSE(control.isPlaying());
}

TEST_F(TransportControlTest, FWI01_ShowsThePositionInSeconds)
{
    control.togglePlay();
    audioBlock(24000);

    EXPECT_DOUBLE_EQ(control.positionSeconds(), 0.5);
}

TEST_F(TransportControlTest, FWI01_NoDeviceNoPosition)
{
    audioBlock(10);
    sampleRate = 0.0;

    EXPECT_DOUBLE_EQ(control.positionSeconds(), 0.0);
    control.locate(2.0);
    audioBlock(10);
    EXPECT_EQ(transport.position(), 0);
}

TEST_F(TransportControlTest, FWI01_LocatesAndReturnsToTheStart)
{
    control.locate(1.5);
    audioBlock(10);
    EXPECT_EQ(transport.position(), 72000);

    control.returnToStart();
    audioBlock(10);
    EXPECT_EQ(transport.position(), 0);
}

TEST_F(TransportControlTest, FWI04_PositionCannotMoveWhileRecording)
{
    recording = true;

    control.locate(1.5);
    audioBlock(10);

    EXPECT_EQ(transport.position(), 0);
}

} // namespace
} // namespace snotes::ui
