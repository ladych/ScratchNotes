#pragma once

#include "notation/Score.h"

#include <cstdint>
#include <optional>
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

/// Where a written note starts and ends in the score, in ticks from its start.
struct NoteSpan
{
    std::int64_t start = 0;
    std::int64_t end = 0;

    bool operator==(const NoteSpan&) const = default;
};

/// Every bar of the MusicXML has the id kBarIdPrefix and its number, e.g. "bar3".
inline constexpr const char* kBarIdPrefix = "bar";

/// The id of a written note in the MusicXML starts with this text, e.g. "n24_30", and goes on with "_" and
/// the rest of its WrittenNote; Verovio keeps it in the SVG, so the cursor finds the notes there (F-NA-04).
[[nodiscard]] std::string noteId(const NoteSpan& span);
/// The span of a note from its id; none for any other id.
[[nodiscard]] std::optional<NoteSpan> noteSpanOf(std::string_view id);

/// What the id of a written note tells, so that a click on it can correct it (F-KO-04).
struct WrittenNote
{
    NoteSpan span;
    /// Index of the voice's part.
    int part = 0;
    /// 1 for the staff of notes, 2 for the tablature.
    int staff = 1;
    /// Place of the note in its chord.
    int index = 0;
    int pitch = 0;
    /// ScoreEvent::seconds of its event, to a microsecond.
    double seconds = 0.0;

    bool operator==(const WrittenNote&) const = default;
};

/// The id of the note, e.g. "n24_30_0_1_0_40_1500000".
[[nodiscard]] std::string writtenNoteId(const WrittenNote& note);
/// The note from its id; none for any other id.
[[nodiscard]] std::optional<WrittenNote> writtenNoteOf(std::string_view id);

/// MusicXML 4.0 of the score (F-NA-01, F-NA-02): a part per voice with a staff of notes in its clef an
/// octave down and a staff of tablature in its tuning, the upbeat as an implicit bar 0. Notes and rests are
/// tied across beats and bars; on the staff of notes eighths and shorter are beamed and triplets bracketed
/// per beat. Rests on the tablature staff are hidden.
[[nodiscard]] std::string toMusicXml(const Score& score);

} // namespace snotes::notation
