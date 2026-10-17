#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace snotes::engine
{

/// Lock-free single-producer/single-consumer ring of samples between the audio thread (writes) and the
/// writer thread (reads). The memory is allocated on construction, on the GUI thread; write() and read()
/// neither allocate nor block (Q-04).
class AudioRingBuffer
{
public:
    explicit AudioRingBuffer(std::size_t capacity);

    /// Producer: appends up to `count` samples and returns how many fit.
    std::size_t write(const float* samples, std::size_t count) noexcept;
    /// Producer: appends up to `count` zeros and returns how many fit.
    std::size_t writeSilence(std::size_t count) noexcept;
    /// Consumer: moves up to `maxCount` samples into `destination` and returns how many.
    std::size_t read(float* destination, std::size_t maxCount) noexcept;

    [[nodiscard]] std::size_t capacity() const noexcept { return data_.size(); }
    /// Consumer: samples waiting to be read.
    [[nodiscard]] std::size_t readable() const noexcept;

private:
    template <typename Fill> std::size_t append(std::size_t count, Fill fill) noexcept;

    std::vector<float> data_;
    // Running totals; their difference is the fill level, their remainder the position in data_.
    std::atomic<std::uint64_t> written_{0};
    std::atomic<std::uint64_t> read_{0};
};

} // namespace snotes::engine
