#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace snotes::engine
{

/// The samples of one take channel, loaded into memory before it plays (F-WI-02). Immutable, so the audio
/// thread reads it without locks; the GUI thread owns and frees it.
class TakeAudio
{
public:
    explicit TakeAudio(std::vector<float> samples) : samples_(std::move(samples)) {}

    [[nodiscard]] std::int64_t length() const noexcept { return static_cast<std::int64_t>(samples_.size()); }
    /// Audio thread: copies numSamples samples from `start` on into `destination`; samples outside the take
    /// are silence.
    void read(std::int64_t start, int numSamples, float* destination) const noexcept;

private:
    std::vector<float> samples_;
};

} // namespace snotes::engine
