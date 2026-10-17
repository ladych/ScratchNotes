#pragma once

#include "engine/RecordingSession.h"
#include "io/WavWriter.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace snotes::io
{

/// The writer thread's side of a recording pass (Pflichtenheft chapter 3): moves the samples from the
/// session's ring buffers into one WAV file per channel and keeps the headers current.
class TakeFileWriter
{
public:
    /// Header updates about once a second at 48 kHz.
    static constexpr std::uint64_t kDefaultHeaderInterval = 48000;

    /// `writers` has one entry per channel of `session`, in the same order.
    TakeFileWriter(engine::RecordingSession& session,
                   std::vector<std::unique_ptr<WavWriter>> writers,
                   std::uint64_t headerInterval = kDefaultHeaderInterval);

    /// Writer thread: writes everything the audio thread has recorded so far.
    void pump();
    /// After the audio thread released the session: writes the rest and the final headers. Returns false
    /// if any write failed during the take.
    bool finish();

    [[nodiscard]] bool failed() const noexcept { return failed_.load(); }

private:
    void updateHeaders();

    engine::RecordingSession& session_;
    std::vector<std::unique_ptr<WavWriter>> writers_;
    std::uint64_t headerInterval_;
    std::uint64_t samplesSinceHeader_ = 0;
    std::vector<float> buffer_;
    std::atomic<bool> failed_{false};
};

} // namespace snotes::io
