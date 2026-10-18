#include "engine/Transport.h"

#include <gtest/gtest.h>

namespace snotes::engine
{
namespace
{

TEST(TransportTest, FWI01_StoppedTransportStaysAtTheStart)
{
    Transport transport;

    const auto block = transport.beginBlock();
    transport.endBlock(block, 64);

    EXPECT_FALSE(block.playing);
    EXPECT_EQ(block.position, 0);
    EXPECT_EQ(transport.position(), 0);
    EXPECT_FALSE(transport.isPlaying());
}

TEST(TransportTest, FWI01_PlayingAdvancesBlockByBlock)
{
    Transport transport;
    transport.play();

    auto block = transport.beginBlock();
    transport.endBlock(block, 64);
    EXPECT_TRUE(block.playing);
    EXPECT_EQ(block.position, 0);

    block = transport.beginBlock();
    transport.endBlock(block, 64);
    EXPECT_EQ(block.position, 64);
    EXPECT_EQ(transport.position(), 128);
    EXPECT_TRUE(transport.isPlaying());
}

TEST(TransportTest, FWI01_StopKeepsThePosition)
{
    Transport transport;
    transport.play();
    transport.endBlock(transport.beginBlock(), 100);

    transport.stop();
    const auto block = transport.beginBlock();
    transport.endBlock(block, 100);

    EXPECT_FALSE(block.playing);
    EXPECT_EQ(transport.position(), 100);
}

TEST(TransportTest, FWI01_LocateTakesEffectWithTheNextBlock)
{
    Transport transport;
    transport.play();
    transport.locate(4800);
    EXPECT_EQ(transport.position(), 0);

    const auto block = transport.beginBlock();
    EXPECT_EQ(block.position, 4800);
    transport.endBlock(block, 10);

    EXPECT_EQ(transport.beginBlock().position, 4810);
}

TEST(TransportTest, FWI01_NegativePositionsMeanTheStart)
{
    Transport transport;
    transport.play();
    transport.endBlock(transport.beginBlock(), 100);

    transport.locate(-5);

    EXPECT_EQ(transport.beginBlock().position, 0);
}

} // namespace
} // namespace snotes::engine
