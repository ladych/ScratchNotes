#pragma once

#include <atomic>

namespace snotes::engine
{

/// Sine test tone used to verify the audio path (milestone M0).
/// render() runs on the audio thread: no allocation, no locks.
class TestToneSource
{
public:
    TestToneSource(double frequencyHz, float gain) noexcept;

    /// Called before playback starts and whenever the sample rate changes.
    void prepare(double sampleRate) noexcept;

    /// Thread-safe; typically called from the GUI thread.
    void setEnabled(bool enabled) noexcept;
    [[nodiscard]] bool isEnabled() const noexcept;

    /// Writes the same signal to every channel. Null channel pointers are
    /// inactive device outputs and are skipped.
    void render(float* const* channels, int numChannels, int numSamples) noexcept;

private:
    [[nodiscard]] float nextSample() noexcept;

    double frequencyHz_;
    float gain_;
    double phase_ = 0.0;
    double phaseIncrement_ = 0.0;
    std::atomic<bool> enabled_{false};
};

} // namespace snotes::engine
