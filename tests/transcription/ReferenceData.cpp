#include "transcription/ReferenceData.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>

namespace snotes::transcription::test
{
namespace
{

std::string path(const std::string& name)
{
    return std::string{SNOTES_TEST_DATA_DIR} + "/transcription/" + name;
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

Activations readActivations(const std::string& prefix)
{
    Activations activations;
    activations.frames = readFloats(prefix + "-frames.f32");
    activations.onsets = readFloats(prefix + "-onsets.f32");
    activations.numFrames = static_cast<int>(activations.frames.size() / basicpitch::kNumPitches);
    return activations;
}

std::vector<ReferenceNote> readNotes(const std::string& prefix)
{
    std::ifstream file{path(prefix + "-notes.csv")};
    std::string line;
    std::getline(file, line);
    std::vector<ReferenceNote> notes;
    while (std::getline(file, line))
    {
        std::istringstream fields{line};
        ReferenceNote reference;
        char comma = 0;
        fields >> reference.note.startFrame >> comma >> reference.note.endFrame >> comma >>
            reference.note.pitch >> comma >> reference.note.amplitude >> comma >> reference.startSeconds >>
            comma >> reference.endSeconds;
        notes.push_back(reference);
    }
    return notes;
}

std::string modelFile()
{
    return std::string{SNOTES_MODEL_FILE};
}

} // namespace snotes::transcription::test
