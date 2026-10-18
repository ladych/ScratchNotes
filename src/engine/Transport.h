#pragma once

#include <atomic>
#include <cstdint>

namespace snotes::engine
{

/// What the audio thread plays in one block.
struct TransportBlock
{
    bool playing = false;
    /// Song position of the block's first sample, in device samples from the song start.
    std::int64_t position = 0;
};

/// Play, stop and the song position (F-WI-01). The GUI thread requests, the audio thread applies the
/// requests at the start of its next block and advances the position; everything is atomic, so neither
/// side waits (Q-04).
class Transport
{
public:
    // GUI thread
    void play() noexcept { playing_.store(true); }
    /// Stops; the position stays where it is.
    void stop() noexcept { playing_.store(false); }
    /// Moves to a song position; negative positions mean the song start.
    void locate(std::int64_t position) noexcept;
    [[nodiscard]] bool isPlaying() const noexcept { return playing_.load(); }
    /// Position as of the last audio block.
    [[nodiscard]] std::int64_t position() const noexcept { return position_.load(); }

    // Audio thread
    /// Applies the requests made since the last block.
    [[nodiscard]] TransportBlock beginBlock() noexcept;
    /// Advances the position by the block if it played.
    void endBlock(const TransportBlock& block, int numSamples) noexcept;

private:
    static constexpr std::int64_t kNoLocate = -1;

    std::atomic<bool> playing_{false};
    std::atomic<std::int64_t> locate_{kNoLocate};
    std::atomic<std::int64_t> position_{0};
};

} // namespace snotes::engine
