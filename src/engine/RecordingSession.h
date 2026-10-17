#pragma once

#include "engine/AudioRingBuffer.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace snotes::engine
{

/// The inputs of one recording pass (F-AU-02, F-AU-05): one ring buffer per recorded input channel. All
/// channels start and stop in the same audio block, so the takes of voices recorded together line up to
/// the sample. Created on the GUI thread, filled by the audio thread, emptied by the writer thread.
class RecordingSession
{
public:
    /// About four seconds at 48 kHz: the writer thread may stall that long without losing audio.
    static constexpr std::size_t kDefaultRingCapacity = 1U << 18U;

    explicit RecordingSession(const std::vector<int>& inputs,
                              std::size_t ringCapacity = kDefaultRingCapacity);

    [[nodiscard]] std::size_t numChannels() const noexcept { return channels_.size(); }
    [[nodiscard]] int input(std::size_t channel) const noexcept { return inputs_[channel]; }
    [[nodiscard]] AudioRingBuffer& ring(std::size_t channel) noexcept { return *channels_[channel]; }

    /// Audio thread: appends one block whose first sample is at song position `position`. An input the device
    /// does not have records silence. Samples that do not fit because the writer thread fell behind are
    /// counted as dropped.
    void record(const float* const* inputs, int numInputs, int numSamples, std::int64_t position) noexcept;

    [[nodiscard]] std::int64_t recordedSamples() const noexcept { return recorded_.load(); }
    [[nodiscard]] std::int64_t droppedSamples() const noexcept { return dropped_.load(); }
    /// Song position of the first recorded sample (F-WI-04), kNotStarted before the first block.
    [[nodiscard]] std::int64_t startSample() const noexcept { return start_.load(); }

    static constexpr std::int64_t kNotStarted = -1;

private:
    std::vector<int> inputs_;
    std::vector<std::unique_ptr<AudioRingBuffer>> channels_;
    std::atomic<std::int64_t> recorded_{0};
    std::atomic<std::int64_t> dropped_{0};
    std::atomic<std::int64_t> start_{kNotStarted};
};

} // namespace snotes::engine
