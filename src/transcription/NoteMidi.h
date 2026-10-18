#pragma once

#include "transcription/Note.h"

#include <cstdint>
#include <vector>

namespace snotes::transcription
{

/// Ticks per quarter note of the MIDI files; at the fixed 120 BPM one second has twice as many.
inline constexpr int kMidiTicksPerQuarter = 960;

/// The notes as a standard MIDI file with one track at 120 BPM (F-EX-01 preview for the command line tool):
/// the times in seconds stay as they were played, the velocity follows the amplitude.
[[nodiscard]] std::vector<std::uint8_t> notesToMidi(const std::vector<Note>& notes);

} // namespace snotes::transcription
