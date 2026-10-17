#pragma once

#include <array>
#include <atomic>

namespace snotes::engine
{

/// Peak level of every input channel for the meters (F-AU-03). The audio thread raises the peaks, the GUI
/// thread takes and resets them; both lock-free.
class InputMeter
{
public:
    static constexpr int kMaxInputs = 64;

    /// Audio thread. Channels beyond kMaxInputs and null channels are ignored.
    void process(const float* const* inputs, int numInputs, int numSamples) noexcept;
    /// GUI thread: the highest absolute sample since the last call, 0 for unknown channels.
    [[nodiscard]] float takePeak(int input) noexcept;

private:
    std::array<std::atomic<float>, kMaxInputs> peaks_{};
};

} // namespace snotes::engine
