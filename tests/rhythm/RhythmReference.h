#pragma once

#include "transcription/Note.h"

#include <string>
#include <vector>

namespace snotes::rhythm::test
{

/// Reads tests/data/rhythm/<name>: little-endian float32 values (tools/export-rhythm-reference.py).
[[nodiscard]] std::vector<float> readFloats(const std::string& name);
/// The rows of a CSV file below its header, as numbers.
[[nodiscard]] std::vector<std::vector<double>> readRows(const std::string& name);
/// The notes the PoC analysed: <prefix>-notes.csv.
[[nodiscard]] std::vector<transcription::Note> readNotes(const std::string& prefix);
/// Beat times of <prefix>-beats.csv: all (used = false) or the ones around the notes.
[[nodiscard]] std::vector<double> readBeats(const std::string& prefix, bool used);
/// The fields of the rows of a CSV file below its header, as text.
[[nodiscard]] std::vector<std::vector<std::string>> readTextRows(const std::string& name);

} // namespace snotes::rhythm::test
