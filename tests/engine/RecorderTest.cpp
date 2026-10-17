#include "engine/Recorder.h"
#include "engine/RecordingSession.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <functional>
#include <utility>
#include <vector>

namespace snotes::engine
{
namespace
{

using ::testing::ElementsAre;

/// Two device inputs with recognisable blocks of two samples.
class Inputs
{
public:
    Inputs(float first, float second) : left_{first, first}, right_{second, second} {}

    [[nodiscard]] const float* const* channels() const { return channels_.data(); }

private:
    std::array<float, 2> left_;
    std::array<float, 2> right_;
    std::array<const float*, 2> channels_{left_.data(), right_.data()};
};

std::vector<float> readAll(AudioRingBuffer& ring)
{
    std::vector<float> samples(ring.capacity());
    samples.resize(ring.read(samples.data(), samples.size()));
    return samples;
}

TEST(RecordingSessionTest, FAU05_RecordsEveryChannelFromItsInput)
{
    RecordingSession session{{1, 0}, 8};
    const Inputs block{0.25F, 0.5F};

    session.record(block.channels(), 2, 2);

    ASSERT_EQ(session.numChannels(), 2U);
    EXPECT_EQ(session.input(0), 1);
    EXPECT_THAT(readAll(session.ring(0)), ElementsAre(0.5F, 0.5F));
    EXPECT_THAT(readAll(session.ring(1)), ElementsAre(0.25F, 0.25F));
    EXPECT_EQ(session.recordedSamples(), 2);
    EXPECT_EQ(session.droppedSamples(), 0);
}

TEST(RecordingSessionTest, FAU02_MissingInputRecordsSilence)
{
    RecordingSession session{{5, -1}, 8};
    const Inputs block{0.25F, 0.5F};

    session.record(block.channels(), 2, 2);

    EXPECT_THAT(readAll(session.ring(0)), ElementsAre(0.0F, 0.0F));
    EXPECT_THAT(readAll(session.ring(1)), ElementsAre(0.0F, 0.0F));
}

TEST(RecordingSessionTest, FAU02_InactiveDeviceChannelRecordsSilence)
{
    RecordingSession session{{0}, 8};
    const std::array<const float*, 1> channels{nullptr};

    session.record(channels.data(), 1, 2);

    EXPECT_THAT(readAll(session.ring(0)), ElementsAre(0.0F, 0.0F));
}

TEST(RecordingSessionTest, FAU02_CountsSamplesThatDidNotFit)
{
    RecordingSession session{{0, 1}, 3};
    const Inputs block{0.25F, 0.5F};

    session.record(block.channels(), 2, 2);
    session.record(block.channels(), 2, 2);

    EXPECT_EQ(session.recordedSamples(), 4);
    EXPECT_EQ(session.droppedSamples(), 2);
}

TEST(RecorderTest, FAU02_RecordsOnlyBetweenStartAndStop)
{
    Recorder recorder;
    RecordingSession session{{0}, 16};
    const Inputs before{0.1F, 0.0F};
    const Inputs during{0.2F, 0.0F};
    const Inputs after{0.3F, 0.0F};

    recorder.process(before.channels(), 2, 2);
    recorder.start(session);
    EXPECT_FALSE(recorder.isReleased(session));
    recorder.process(during.channels(), 2, 2);
    recorder.stop();
    recorder.process(after.channels(), 2, 2);

    EXPECT_TRUE(recorder.isReleased(session));
    EXPECT_THAT(readAll(session.ring(0)), ElementsAre(0.2F, 0.2F));
}

/// Runs a hook right after the next load, i.e. between the audio thread's read of the session and its
/// announcement in the hazard pointer.
std::function<void()> runAfterNextLoad;

class RacingAtomic
{
public:
    explicit RacingAtomic(RecordingSession* value) : value_(value) {}

    RecordingSession* load() const
    {
        RecordingSession* const value = value_;
        if (auto hook = std::exchange(runAfterNextLoad, nullptr))
            hook();
        return value;
    }

    void store(RecordingSession* value) { value_ = value; }

private:
    RecordingSession* value_;
};

TEST(RecorderTest, Q04_SessionStoppedWhileTheBlockStartsIsNotTouched)
{
    BasicRecorder<RacingAtomic> recorder;
    RecordingSession session{{0}, 16};
    const Inputs block{0.2F, 0.0F};
    recorder.start(session);

    // The GUI thread stops right after the audio thread read the session but before it announced it,
    // and then considers the session released: the audio thread must not write into it.
    bool releasedDuringBlock = false;
    runAfterNextLoad = [&]
    {
        recorder.stop();
        releasedDuringBlock = recorder.isReleased(session);
    };
    recorder.process(block.channels(), 2, 2);

    EXPECT_TRUE(releasedDuringBlock);
    EXPECT_EQ(session.ring(0).readable(), 0U);
}

TEST(RecorderTest, Q04_SessionInUseIsNotReleased)
{
    BasicRecorder<RacingAtomic> recorder;
    RecordingSession session{{0}, 16};
    const Inputs block{0.2F, 0.0F};
    recorder.start(session);

    // The GUI thread stops after the audio thread announced the session: the block is still written,
    // and the session counts as released only after the block.
    bool releasedDuringBlock = true;
    runAfterNextLoad = [&]
    {
        runAfterNextLoad = [&]
        {
            recorder.stop();
            releasedDuringBlock = recorder.isReleased(session);
        };
    };
    recorder.process(block.channels(), 2, 2);

    EXPECT_FALSE(releasedDuringBlock);
    EXPECT_TRUE(recorder.isReleased(session));
    EXPECT_THAT(readAll(session.ring(0)), ElementsAre(0.2F, 0.2F));
}

} // namespace
} // namespace snotes::engine
