#pragma once

#include "transcription/Note.h"

#include <optional>
#include <string>
#include <vector>

namespace snotes::transcription
{

/// The notes as CSV with the header "start,end,pitch,amplitude": times in seconds, pitch as MIDI note. The
/// command line tool and the program store transcriptions in this form until the score model of AP5/AP7.
[[nodiscard]] std::string notesToCsv(const std::vector<Note>& notes);
/// Reads what notesToCsv wrote; nothing if a line is not a note.
[[nodiscard]] std::optional<std::vector<Note>> notesFromCsv(const std::string& csv);

} // namespace snotes::transcription
