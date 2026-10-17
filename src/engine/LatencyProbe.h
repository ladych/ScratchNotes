#pragma once

#include <array>
#include <atomic>
#include <cstdint>

namespace snotes::engine
{

/// Built-in round-trip measurement (Q-01, Q-05): with a cable from an output to an input it sends a
/// short click to all outputs and looks for the loudest sample at the input within half a second, several
/// times. The distance in samples is the true round trip including converters and filters, which the
/// drivers often report wrongly. Audio thread work is allocation-free (Q-04).
class LatencyProbe
{
public:
    static constexpr int kPings = 5;
    static constexpr double kWindowSeconds = 0.5;
    static constexpr float kPingLevel = 0.5F;
    /// Peaks below this count as "nothing arrived" (no cable, wrong input).
    static constexpr float kMinPeak = 0.01F;
    static constexpr std::int64_t kNoPeak = -1;

    // GUI thread
    /// Starts a measurement listening on `input`; earlier results are cleared.
    void start(int input) noexcept;
    /// Aborts a running measurement.
    void cancel() noexcept { active_.store(false); }
    [[nodiscard]] bool isRunning() const noexcept { return active_.load(); }
    /// Number of pings finished so far.
    [[nodiscard]] int numFinished() const noexcept { return finished_.load(); }
    /// Round trip of ping `index` in samples, or kNoPeak if nothing arrived.
    [[nodiscard]] std::int64_t result(int index) const noexcept;

    // Audio thread
    void prepare(double sampleRate) noexcept;
    /// Adds the pings to the outputs and scans the input; does nothing while no measurement runs.
    void process(const float* const* inputs,
                 int numInputs,
                 float* const* outputs,
                 int numOutputs,
                 int numSamples) noexcept;

private:
    void scan(const float* input, int sample) noexcept;
    void finishPing() noexcept;

    std::atomic<bool> active_{false};
    std::atomic<int> input_{0};
    std::atomic<int> finished_{0};
    std::atomic<bool> restart_{false};
    std::array<std::atomic<std::int64_t>, kPings> results_{};
    // Audio thread only
    std::int64_t window_ = 24000;
    std::int64_t elapsed_ = 0;
    std::int64_t peakAt_ = kNoPeak;
    float peak_ = 0.0F;
};

} // namespace snotes::engine
