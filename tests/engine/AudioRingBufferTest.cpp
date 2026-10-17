#include "engine/AudioRingBuffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <vector>

namespace snotes::engine
{
namespace
{

using ::testing::ElementsAre;

std::vector<float> readAll(AudioRingBuffer& ring)
{
    std::vector<float> samples(ring.capacity());
    samples.resize(ring.read(samples.data(), samples.size()));
    return samples;
}

TEST(AudioRingBufferTest, FAU02_ReadsWhatWasWritten)
{
    AudioRingBuffer ring{8};
    const std::array<float, 3> block{0.1F, 0.2F, 0.3F};

    EXPECT_EQ(ring.write(block.data(), block.size()), 3U);
    EXPECT_EQ(ring.readable(), 3U);

    EXPECT_THAT(readAll(ring), ElementsAre(0.1F, 0.2F, 0.3F));
    EXPECT_EQ(ring.readable(), 0U);
}

TEST(AudioRingBufferTest, FAU02_WrapsAroundTheEnd)
{
    AudioRingBuffer ring{4};
    const std::array<float, 3> first{1.0F, 2.0F, 3.0F};
    const std::array<float, 3> second{4.0F, 5.0F, 6.0F};
    ring.write(first.data(), first.size());
    static_cast<void>(readAll(ring));

    EXPECT_EQ(ring.write(second.data(), second.size()), 3U);

    EXPECT_THAT(readAll(ring), ElementsAre(4.0F, 5.0F, 6.0F));
}

TEST(AudioRingBufferTest, FAU02_FullRingTakesOnlyWhatFits)
{
    AudioRingBuffer ring{4};
    const std::array<float, 6> block{1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F};

    EXPECT_EQ(ring.write(block.data(), block.size()), 4U);
    EXPECT_EQ(ring.write(block.data(), 1), 0U);

    EXPECT_THAT(readAll(ring), ElementsAre(1.0F, 2.0F, 3.0F, 4.0F));
}

TEST(AudioRingBufferTest, FAU02_ReadTakesAtMostTheRequestedCount)
{
    AudioRingBuffer ring{4};
    const std::array<float, 3> block{1.0F, 2.0F, 3.0F};
    ring.write(block.data(), block.size());
    std::array<float, 2> target{};

    EXPECT_EQ(ring.read(target.data(), target.size()), 2U);

    EXPECT_THAT(target, ElementsAre(1.0F, 2.0F));
    EXPECT_EQ(ring.readable(), 1U);
}

TEST(AudioRingBufferTest, FAU02_WritesSilence)
{
    AudioRingBuffer ring{4};
    const std::array<float, 3> block{1.0F, 2.0F, 3.0F};
    ring.write(block.data(), block.size());
    static_cast<void>(readAll(ring));

    EXPECT_EQ(ring.writeSilence(3), 3U);

    EXPECT_THAT(readAll(ring), ElementsAre(0.0F, 0.0F, 0.0F));
}

} // namespace
} // namespace snotes::engine
