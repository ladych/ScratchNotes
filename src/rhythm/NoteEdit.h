#pragma once

#include "rhythm/Quantizer.h"

#include <cstdint>
#include <vector>

namespace snotes::rhythm
{

/// A correction of a note by the user (F-KO-04, F-TB-03). It finds its note by the start of the event in
/// seconds, which stays the same when the grid changes, so it survives a new quantization (F-KO-05).
struct NoteEdit
{
    static constexpr int kNone = -1;
    /// Ticks per beat of durationTicks, as in the score.
    static constexpr std::int64_t kTicksPerBeat = 12;

    /// QuantizedEvent::seconds of the event.
    double seconds = 0.0;
    /// The detected pitch of the note; kNone for an edit of the whole event.
    int pitch = kNone;
    bool deleted = false;
    /// The pitch it is written with; kNone to keep it.
    int toPitch = kNone;
    /// The string it is played on, 0 for the lowest; kNone to let the fingering choose.
    int string = kNone;
    /// Length of the event; 0 to keep it.
    std::int64_t durationTicks = 0;

    /// Same note: the same pitch of an event within 0.1 ms.
    [[nodiscard]] bool sameNote(const NoteEdit& other) const;

    bool operator==(const NoteEdit&) const = default;
};

/// The events after the edits and the strings their pitches are fixed to.
struct EditedEvents
{
    std::vector<QuantizedEvent> events;
    /// Per event the string of each pitch, NoteEdit::kNone if it is free; empty if all are free.
    std::vector<std::vector<int>> strings;

    bool operator==(const EditedEvents&) const = default;
};

/// Applies the edits: deleted notes go, events without notes too; transposed notes get their new pitch and
/// the pitches stay ascending. Edits of notes the events do not have are ignored.
[[nodiscard]] EditedEvents applyNoteEdits(const std::vector<QuantizedEvent>& events,
                                          const std::vector<NoteEdit>& edits);

} // namespace snotes::rhythm
