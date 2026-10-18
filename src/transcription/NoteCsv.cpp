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

} // namespace snotes::transcription
