#pragma once

#include "io/IBinaryFile.h"

#include <cstdint>
#include <memory>
#include <span>

namespace snotes::io
{

/// Writes a mono WAV file with 32-bit float samples (F-AU-02) while it is being recorded. The header is
/// written first and its sizes are brought up to date by updateHeader(), so the file stays readable up to
/// the last update if the program crashes during a take (Pflichtenheft chapter 4).
///
/// The RIFF sizes are 32 bit, so a take holds at most about 6 hours at 48 kHz.
class WavWriter
{
public:
    static constexpr std::uint32_t kHeaderSize = 58;

    WavWriter(std::unique_ptr<IBinaryFile> file, double sampleRate);

    bool append(std::span<const float> samples);
    /// Writes the current sizes into the header and flushes the file.
    bool updateHeader();

    [[nodiscard]] std::uint64_t numSamples() const noexcept { return numSamples_; }
    /// False once a write failed; later writes are skipped.
    [[nodiscard]] bool ok() const noexcept { return ok_; }

private:
    bool check(bool written);

    std::unique_ptr<IBinaryFile> file_;
    std::uint64_t numSamples_ = 0;
    bool ok_ = true;
};

} // namespace snotes::io
