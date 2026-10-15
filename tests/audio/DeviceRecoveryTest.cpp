#include "audio/DeviceRecovery.h"

#include <gtest/gtest.h>

namespace snotes::audio
{
namespace
{

constexpr std::size_t kListWithInterface = 1;
constexpr std::size_t kListWithoutInterface = 2;

/// Observations of one test, with the block counter running like a playing device.
class DeviceRecoveryTest : public ::testing::Test
{
protected:
    RecoveryAction tick() { return recovery_.tick(observation_); }
    void play() { observation_.audioBlocks += 100; }
    void playingOnPreferred()
    {
        observation_ = {.hasPreferred = true,
                        .open = true,
                        .onPreferred = true,
                        .audioBlocks = observation_.audioBlocks,
                        .deviceList = kListWithInterface};
        play();
    }
    void playingOnFallback(std::size_t deviceList)
    {
        observation_.open = true;
        observation_.onPreferred = false;
        observation_.deviceList = deviceList;
        play();
    }

    DeviceRecovery recovery_;
    DeviceObservation observation_;
};

TEST_F(DeviceRecoveryTest, Q09_PlayingPreferredDeviceNeedsNothing)
{
    for (int second = 0; second < 10; ++second)
    {
        playingOnPreferred();
        EXPECT_EQ(tick(), RecoveryAction::none);
    }
    EXPECT_FALSE(recovery_.stalled());
}

TEST_F(DeviceRecoveryTest, Q09_SilentDeviceIsReopenedAfterThreeSeconds)
{
    playingOnPreferred();
    tick();
    EXPECT_EQ(tick(), RecoveryAction::none);
    EXPECT_FALSE(recovery_.stalled());
    EXPECT_EQ(tick(), RecoveryAction::none);
    EXPECT_EQ(tick(), RecoveryAction::reopen);
    EXPECT_TRUE(recovery_.stalled());
}

TEST_F(DeviceRecoveryTest, Q09_SilentDeviceIsRetriedEveryFiveSeconds)
{
    playingOnPreferred();
    tick();
    for (int second = 0; second < 3; ++second)
        tick();
    for (int second = 0; second < 4; ++second)
        EXPECT_EQ(tick(), RecoveryAction::none);
    EXPECT_EQ(tick(), RecoveryAction::reopen);
}

TEST_F(DeviceRecoveryTest, Q09_RunningAgainEndsTheStall)
{
    playingOnPreferred();
    tick();
    for (int second = 0; second < 3; ++second)
        tick();
    play();
    EXPECT_EQ(tick(), RecoveryAction::none);
    EXPECT_FALSE(recovery_.stalled());
}

TEST_F(DeviceRecoveryTest, Q09_ClosedDeviceIsRetriedLikeASilentOne)
{
    observation_ = {.hasPreferred = true};
    tick();
    tick();
    EXPECT_EQ(tick(), RecoveryAction::reopen);
    EXPECT_TRUE(recovery_.stalled());
    observation_.deviceList = kListWithoutInterface;
    EXPECT_EQ(tick(), RecoveryAction::none);
}

TEST_F(DeviceRecoveryTest, Q09_FallbackWaitsForTheDeviceListToChange)
{
    playingOnPreferred();
    tick();
    playingOnFallback(kListWithoutInterface);
    for (int second = 0; second < 10; ++second)
    {
        EXPECT_EQ(tick(), RecoveryAction::none);
        play();
    }
}

TEST_F(DeviceRecoveryTest, Q09_PluggingTheInterfaceBackReturnsToIt)
{
    playingOnPreferred();
    tick();
    playingOnFallback(kListWithoutInterface);
    tick();
    playingOnFallback(kListWithInterface);
    EXPECT_EQ(tick(), RecoveryAction::reopen);
    // The attempt failed and the fallback plays again: no new attempt until the list changes again.
    playingOnFallback(kListWithInterface);
    EXPECT_EQ(tick(), RecoveryAction::none);
}

TEST_F(DeviceRecoveryTest, Q09_AfterAFailedReopenTheFallbackWaitsForTheList)
{
    observation_ = {.hasPreferred = true, .deviceList = kListWithoutInterface};
    tick();
    tick();
    ASSERT_EQ(tick(), RecoveryAction::reopen);
    playingOnFallback(kListWithoutInterface);
    EXPECT_EQ(tick(), RecoveryAction::none);
    playingOnFallback(kListWithInterface);
    EXPECT_EQ(tick(), RecoveryAction::reopen);
}

TEST_F(DeviceRecoveryTest, Q09_ReturningToThePreferredDeviceForgetsTheTriedList)
{
    playingOnPreferred();
    tick();
    playingOnFallback(kListWithoutInterface);
    tick();
    playingOnPreferred();
    tick();
    // Unplugged again while the list fingerprint is the same as at the first fallback.
    playingOnFallback(kListWithoutInterface);
    EXPECT_EQ(tick(), RecoveryAction::none);
    playingOnFallback(kListWithInterface);
    EXPECT_EQ(tick(), RecoveryAction::reopen);
}

TEST_F(DeviceRecoveryTest, FAO03_WithoutSavedChoiceThereIsNoFallback)
{
    observation_ = {.open = true, .deviceList = kListWithInterface};
    play();
    tick();
    observation_.deviceList = kListWithoutInterface;
    play();
    EXPECT_EQ(tick(), RecoveryAction::none);
}

} // namespace
} // namespace snotes::audio
