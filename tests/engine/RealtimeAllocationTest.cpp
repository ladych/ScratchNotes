#include "engine/SpscQueue.h"
#include "engine/TestToneSource.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <new>

namespace
{

// Counts heap allocations of the current thread while counting is switched on.
thread_local bool countAllocations = false;
thread_local int allocationCount = 0;

} // namespace

void* operator new(std::size_t size)
{
    if (countAllocations)
        ++allocationCount;
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t /*size*/) noexcept
{
    std::free(memory);
}

namespace snotes::engine
{
namespace
{

TEST(RealtimeAllocationTest, Q04_TestToneDoesNotAllocate)
{
    TestToneSource tone{440.0, 0.2F};
    tone.prepare(48000.0);
    tone.setEnabled(true);
    std::array<float, 128> left{};
    std::array<float, 128> right{};
    const std::array<float*, 2> channels{left.data(), right.data()};

    countAllocations = true;
    for (int block = 0; block < 200; ++block)
        tone.render(channels.data(), 2, 128);
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
}

TEST(RealtimeAllocationTest, Q04_QueueDoesNotAllocate)
{
    SpscQueue<int, 64> queue;
    int value = 0;

    countAllocations = true;
    for (int round = 0; round < 1000; ++round)
    {
        queue.push(round);
        queue.pop(value);
    }
    countAllocations = false;

    EXPECT_EQ(allocationCount, 0);
    EXPECT_EQ(value, 999);
}

} // namespace
} // namespace snotes::engine
