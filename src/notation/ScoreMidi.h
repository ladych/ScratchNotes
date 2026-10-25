#pragma once

#include "notation/Score.h"

#include <cstdint>
#include <span>
#include <vector>

namespace snotes::notation
{

/// Ticks per quarter note of the exported MIDI file.
inline constexpr int kMidiTicksPerBeat = 960;

/// The time of a beat on the grid (beat 0 is its first beat), between beats in proportion; before the first
/// and after the last beat the grid goes on with its first and last interval. beats are the times of the grid
/// in seconds, at least two.
[[nodiscard]] double secondsAt(std::span<const double> beats, double beat);

/// The score as a standard MIDI file of type 1 (F-EX-01): a tempo track with the meter and a tempo on every
/// beat, then one track per voice with its name, a guitar or bass sound and its own channel. The file runs in
/// sync with the recording: MIDI time 0 is the start of the project, the lead-in before the score takes as
/// many whole bars as come closest to the tempo, so that bar 1 of the score falls on a bar line of the DAW.
/// Grace notes are thirty-second notes just before their chord. beats as in secondsAt.
[[nodiscard]] std::vector<std::uint8_t> scoreToMidi(const Score& score, std::span<const double> beats);

} // namespace snotes::notation
