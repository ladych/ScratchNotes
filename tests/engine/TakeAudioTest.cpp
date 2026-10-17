#include "engine/TakeAudio.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>

namespace snotes::engine
{
namespace
{

using ::testing::ElementsAre;

const TakeAudio kTake{{1.0F, 2.0F, 3.0F}};

std::array<float, 4> read(std::int64_t start)
{
    std::array<float, 4> samples{9.0F, 9.0F, 9.0F, 9.0F};
    kTake.read(start, 4, samples.data());
    return samples;
}

TEST(TakeAudioTest, FWI02_ReadsTheSamples)
{
    EXPECT_EQ(kTake.length(), 3);
    EXPECT_THAT(read(0), ElementsAre(1.0F, 2.0F, 3.0F, 0.0F));
}

TEST(TakeAudioTest, FWI02_BeforeTheTakeIsSilence)
{
    EXPECT_THAT(read(-2), ElementsAre(0.0F, 0.0F, 1.0F, 2.0F));
}

TEST(TakeAudioTest, FWI02_AfterTheTakeIsSilence)
{
    EXPECT_THAT(read(3), ElementsAre(0.0F, 0.0F, 0.0F, 0.0F));
    EXPECT_THAT(read(-10), ElementsAre(0.0F, 0.0F, 0.0F, 0.0F));
}

} // namespace
} // namespace snotes::engine
