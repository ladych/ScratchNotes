#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace snotes::engine
{

/// Lock-free single-producer/single-consumer ring buffer with fixed capacity.
/// One thread pushes (e.g. GUI), one thread pops (audio); neither allocates nor blocks.
template <typename T, std::size_t Capacity> class SpscQueue
{
public:
    /// Producer side. Returns false if the queue is full.
    bool push(const T& item) noexcept
    {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto next = increment(tail);
        if (next == head_.load(std::memory_order_acquire))
            return false;
        items_.at(tail) = item;
        tail_.store(next, std::memory_order_release);
        return true;
    }

    /// Consumer side. Returns false if the queue is empty.
    bool pop(T& item) noexcept
    {
        const auto head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire))
            return false;
        item = items_.at(head);
        head_.store(increment(head), std::memory_order_release);
        return true;
    }

private:
    static constexpr std::size_t kSlots = Capacity + 1;

    static constexpr std::size_t increment(std::size_t index) noexcept { return (index + 1) % kSlots; }

    std::array<T, kSlots> items_{};
    std::atomic<std::size_t> head_{0};
    std::atomic<std::size_t> tail_{0};
};

} // namespace snotes::engine
