#include "engine/SpscQueue.h"

#include <gtest/gtest.h>

#include <thread>

namespace snotes::engine
{
namespace
{

TEST(SpscQueueTest, Q04_PopsItemsInOrder)
{
    SpscQueue<int, 2> queue;
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    int item = 0;
    EXPECT_TRUE(queue.pop(item));
    EXPECT_EQ(item, 1);
    EXPECT_TRUE(queue.pop(item));
    EXPECT_EQ(item, 2);
    EXPECT_FALSE(queue.pop(item));
}

TEST(SpscQueueTest, Q04_RejectsPushWhenFull)
{
    SpscQueue<int, 2> queue;
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    EXPECT_FALSE(queue.push(3));
    int item = 0;
    EXPECT_TRUE(queue.pop(item));
    EXPECT_TRUE(queue.push(3));
}

TEST(SpscQueueTest, Q04_TransfersItemsBetweenThreads)
{
    constexpr int kCount = 10000;
    SpscQueue<int, 2> queue;
    std::thread producer(
        [&queue]
        {
            for (int i = 0; i < kCount; ++i)
                while (!queue.push(i))
                    std::this_thread::yield();
        });
    int expected = 0;
    while (expected < kCount)
    {
        int item = -1;
        if (queue.pop(item))
        {
            ASSERT_EQ(item, expected);
            ++expected;
        }
    }
    producer.join();
    int item = 0;
    EXPECT_FALSE(queue.pop(item));
}

} // namespace
} // namespace snotes::engine
