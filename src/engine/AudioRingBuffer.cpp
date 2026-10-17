#include "engine/AudioRingBuffer.h"

#include <algorithm>
#include <span>

namespace snotes::engine
{

AudioRingBuffer::AudioRingBuffer(std::size_t capacity) : data_(capacity) {}

std::size_t AudioRingBuffer::write(const float* samples, std::size_t count) noexcept
{
    const std::span source{samples, count};
    return append(count,
                  [source](std::size_t offset, std::span<float> target)
                  { std::ranges::copy(source.subspan(offset, target.size()), target.begin()); });
}

std::size_t AudioRingBuffer::writeSilence(std::size_t count) noexcept
{
    return append(count,
                  [](std::size_t /*offset*/, std::span<float> target) { std::ranges::fill(target, 0.0F); });
}

template <typename Fill> std::size_t AudioRingBuffer::append(std::size_t count, Fill fill) noexcept
{
    const auto written = written_.load(std::memory_order_relaxed);
    const auto free =
        data_.size() - static_cast<std::size_t>(written - read_.load(std::memory_order_acquire));
    const auto total = std::min(count, free);
    const auto start = static_cast<std::size_t>(written % data_.size());
    const auto first = std::min(total, data_.size() - start);
    const std::span ring{data_};
    fill(0, ring.subspan(start, first));
    fill(first, ring.subspan(0, total - first));
    written_.store(written + total, std::memory_order_release);
    return total;
}

std::size_t AudioRingBuffer::read(float* destination, std::size_t maxCount) noexcept
{
    const auto alreadyRead = read_.load(std::memory_order_relaxed);
    const auto total = std::min(maxCount, readable());
    const auto start = static_cast<std::size_t>(alreadyRead % data_.size());
    const auto first = std::min(total, data_.size() - start);
    const std::span ring{data_};
    const std::span target{destination, total};
    std::ranges::copy(ring.subspan(start, first), target.begin());
    std::ranges::copy(ring.subspan(0, total - first), target.subspan(first).begin());
    read_.store(alreadyRead + total, std::memory_order_release);
    return total;
}

std::size_t AudioRingBuffer::readable() const noexcept
{
    return static_cast<std::size_t>(written_.load(std::memory_order_acquire) -
                                    read_.load(std::memory_order_relaxed));
}

} // namespace snotes::engine
