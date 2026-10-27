#pragma once

#include <cstdint>
#include <vector>

namespace snotes::engine
{

/// A note of the transcription as sound (F-NA-05), in samples of the song.
struct SynthNote
{
    std::int64_t start = 0;
    std::int64_t end = 0;
    double frequency = 0.0;

    bool operator==(const SynthNote&) const = default;
};

/// The notes of a voice in the order of their starts, and the length of the longest, so that the notes that
/// sound in a block are found without looking at all of them.
struct SynthNotes
{
    std::vector<SynthNote> notes;
    std::int64_t longest = 0;
};

/// Plays the transcription with a plucked tone (F-NA-05): three harmonics that die away, a short attack and
/// a short release at the end of the note. Derived from the song position alone, like the click, so it
/// sounds right after every locate. Audio thread only, no allocation (Q-04).
class NoteSynth
{
public:
    static constexpr double kAttackSeconds = 0.005;
    /// Time in which a note dies away to a third.
    static constexpr double kDecaySeconds = 0.8;
    static constexpr double kReleaseSeconds = 0.05;
    static constexpr float kLevel = 0.3F;

    /// Adds the notes that sound from song sample `position` on, times gain, to all outputs.
    static void render(const SynthNotes& notes,
                       double sampleRate,
                       std::int64_t position,
                       float gain,
                       float* const* outputs,
                       int numOutputs,
                       int numSamples) noexcept;

    /// The value of a note at a song sample; 0 before it and after its release.
    [[nodiscard]] static float
    sampleAt(const SynthNote& note, std::int64_t sample, double sampleRate) noexcept;
};

} // namespace snotes::engine
