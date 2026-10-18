#include "rhythm/RhythmReference.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>

namespace snotes::rhythm::test
{
namespace
{

std::string path(const std::string& name)
{
    return std::string{SNOTES_TEST_DATA_DIR} + "/rhythm/" + name;
}

std::vector<std::string> split(const std::string& line)
{
    std::vector<std::string> fields;
    std::istringstream stream{line};
    std::string field;
    while (std::getline(stream, field, ','))
        fields.push_back(field);
    if (!line.empty() && line.back() == ',')
        fields.emplace_back();
    return fields;
}

std::vector<double> numbers(const std::string& line)
{
    std::vector<double> values;
    for (const auto& field : split(line))
        values.push_back(std::stod(field));
    return values;
}

} // namespace

std::vector<float> readFloats(const std::string& name)
{
    std::ifstream file{path(name), std::ios::binary};
    const std::vector<char> bytes{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    std::vector<float> values(bytes.size() / sizeof(float));
    std::memcpy(values.data(), bytes.data(), values.size() * sizeof(float));
    return values;
}

std::vector<std::vector<std::string>> readTextRows(const std::string& name)
{
    std::ifstream file{path(name)};
    std::string line;
    std::getline(file, line);
    std::vector<std::vector<std::string>> rows;
    while (std::getline(file, line))
        rows.push_back(split(line));
    return rows;
}

std::vector<std::vector<double>> readRows(const std::string& name)
{
    std::ifstream file{path(name)};
    std::string line;
    std::getline(file, line);
    std::vector<std::vector<double>> rows;
    while (std::getline(file, line))
        rows.push_back(numbers(line));
    return rows;
}

std::vector<transcription::Note> readNotes(const std::string& prefix)
{
    std::vector<transcription::Note> notes;
    for (const auto& row : readRows(prefix + "-notes.csv"))
        notes.push_back({.startSeconds = row[0],
                         .endSeconds = row[1],
                         .pitch = static_cast<int>(row[2]),
                         .amplitude = static_cast<float>(row[3])});
    return notes;
}

std::vector<double> readBeats(const std::string& prefix, bool used)
{
    std::ifstream file{path(prefix + "-beats.csv")};
    std::string line;
    for (int skip = used ? 3 : 1; skip > 0; --skip)
        std::getline(file, line);
    std::getline(file, line);
    return numbers(line);
}

} // namespace snotes::rhythm::test
