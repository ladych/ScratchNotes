#include "transcription/NoteCsv.h"

#include <iomanip>
#include <locale>
#include <sstream>

namespace snotes::transcription
{

std::string notesToCsv(const std::vector<Note>& notes)
{
    std::ostringstream csv;
    // Decimal points regardless of the user's locale.
    csv.imbue(std::locale::classic());
    csv << "start,end,pitch,amplitude\n" << std::fixed;
    for (const auto& note : notes)
        csv << std::setprecision(4) << note.startSeconds << ',' << note.endSeconds << ',' << note.pitch << ','
            << std::setprecision(3) << note.amplitude << '\n';
    return csv.str();
}

std::optional<std::vector<Note>> notesFromCsv(const std::string& csv)
{
    std::istringstream lines{csv};
    lines.imbue(std::locale::classic());
    std::string line;
    std::getline(lines, line);
    std::vector<Note> notes;
    while (std::getline(lines, line))
    {
        std::istringstream fields{line};
        fields.imbue(std::locale::classic());
        Note note;
        char first = 0;
        char second = 0;
        char third = 0;
        fields >> note.startSeconds >> first >> note.endSeconds >> second >> note.pitch >> third >>
            note.amplitude;
        if (fields.fail() || first != ',' || second != ',' || third != ',')
            return std::nullopt;
        notes.push_back(note);
    }
    return notes;
}

} // namespace snotes::transcription
