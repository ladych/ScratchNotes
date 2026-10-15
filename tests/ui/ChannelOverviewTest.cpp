#include "ui/ChannelOverview.h"

#include <gtest/gtest.h>

namespace snotes::ui
{
namespace
{

TEST(ChannelOverviewTest, FAO01_NumbersTheChannelsAndMarksTheInactiveOnes)
{
    const auto entries = channelEntries({"Mic/Line 1", "Mic/Line 2", "Hi-Z 3"}, {true, false, true});

    ASSERT_EQ(entries.size(), 3U);
    EXPECT_EQ(entries[0].text, "1  Mic/Line 1");
    EXPECT_TRUE(entries[0].active);
    EXPECT_EQ(entries[1].text, "2  Mic/Line 2 (aus)");
    EXPECT_FALSE(entries[1].active);
    EXPECT_EQ(entries[2].text, "3  Hi-Z 3");
}

TEST(ChannelOverviewTest, FAO01_ChannelsWithoutActiveFlagAreInactive)
{
    const auto entries = channelEntries({"In 1", "In 2"}, {true});

    EXPECT_TRUE(entries[0].active);
    EXPECT_FALSE(entries[1].active);
}

TEST(ChannelOverviewTest, FAO01_UnnamedChannelsGetTheirNumberOnly)
{
    EXPECT_EQ(channelEntries({""}, {true}).front().text, "1");
}

TEST(ChannelOverviewTest, FAO01_NoChannelsGiveNoEntries)
{
    EXPECT_TRUE(channelEntries({}, {}).empty());
}

} // namespace
} // namespace snotes::ui
