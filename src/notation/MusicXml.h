#pragma once

#include "notation/Score.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace snotes::notation
{

/// A written note value: its length in ticks, the MusicXML type, dots and whether it is a triplet.
struct NoteValue
{
    std::int64_t ticks = 0;
    std::string_view type;
    int dots = 0;
    bool triplet = false;

    bool operator==(const NoteValue&) const = default;
};

/// The tied note values of a note or rest that starts start ticks into a bar: the part up to the next beat,
/// whole beats as quarter, half, dotted half and whole notes, then the rest of the last beat. Inside a beat
/// the longest value that fits comes first: dotted eighth, triplet quarter, eighth, triplet eighth,
/// sixteenth, triplet sixteenth, triplet thirty-second.
[[nodiscard]] std::vector<NoteValue> noteValues(std::int64_t start, std::int64_t length);

/// MusicXML 4.0 of the score (F-NA-01, F-NA-02): a part per voice with a staff of notes in its clef an
/// octave down and a staff of tablature in its tuning, the upbeat as an implicit bar 0. Notes and rests are
/// tied across beats and bars; on the staff of notes eighths and shorter are beamed and triplets bracketed
/// per beat. Rests on the tablature staff are hidden.
[[nodiscard]] std::string toMusicXml(const Score& score);

} // namespace snotes::notation
