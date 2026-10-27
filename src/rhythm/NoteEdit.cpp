#include "rhythm/NoteEdit.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace snotes::rhythm
{
namespace
{

constexpr double kSameSeconds = 1e-4;

/// The edit of this pitch of the event, if there is one.
const NoteEdit* findEdit(const std::vector<NoteEdit>& edits, double seconds, int pitch)
{
    const NoteEdit wanted{.seconds = seconds, .pitch = pitch};
    const auto found =
        std::ranges::find_if(edits, [&wanted](const NoteEdit& edit) { return edit.sameNote(wanted); });
    return found == edits.end() ? nullptr : &*found;
}

struct EditedNote
{
    int pitch = 0;
    int string = NoteEdit::kNone;
};

/// The notes of the event after the edits, ascending.
std::vector<EditedNote> editedNotes(const QuantizedEvent& event, const std::vector<NoteEdit>& edits)
{
    std::vector<EditedNote> notes;
    for (const int pitch : event.pitches)
    {
        const auto* const edit = findEdit(edits, event.seconds, pitch);
        if (edit == nullptr)
            notes.push_back({.pitch = pitch});
        else if (!edit->deleted)
            notes.push_back(
                {.pitch = edit->toPitch == NoteEdit::kNone ? pitch : edit->toPitch, .string = edit->string});
    }
    std::ranges::stable_sort(notes, {}, &EditedNote::pitch);
    return notes;
}

} // namespace

bool NoteEdit::sameNote(const NoteEdit& other) const
{
    return pitch == other.pitch && std::abs(seconds - other.seconds) < kSameSeconds;
}

EditedEvents applyNoteEdits(const std::vector<QuantizedEvent>& events, const std::vector<NoteEdit>& edits)
{
    EditedEvents edited;
    for (const auto& event : events)
    {
        const auto notes = editedNotes(event, edits);
        if (notes.empty())
            continue;
        auto changed = event;
        changed.pitches.clear();
        std::vector<int> strings;
        bool fixed = false;
        for (const auto& note : notes)
        {
            changed.pitches.push_back(note.pitch);
            strings.push_back(note.string);
            if (note.string != NoteEdit::kNone)
                fixed = true;
        }
        const auto* const whole = findEdit(edits, event.seconds, NoteEdit::kNone);
        if (whole != nullptr && whole->durationTicks > 0)
            changed.duration = Fraction{whole->durationTicks, NoteEdit::kTicksPerBeat};
        edited.events.push_back(std::move(changed));
        edited.strings.push_back(fixed ? std::move(strings) : std::vector<int>{});
    }
    return edited;
}

} // namespace snotes::rhythm
