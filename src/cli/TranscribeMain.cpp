// scratchnotes-transcribe (Pflichtenheft 3, tools/): detects the notes of DI recordings without the program,
// for checking the note detection against the Python prototype and for batch runs over old recordings.

#include "io/Utf8Path.h"
#include "model/InstrumentProfile.h"
#include "transcription/CommandLine.h"
#include "transcription/NoteCsv.h"
#include "transcription/NoteMidi.h"
#include "transcription/OnnxPitchModel.h"
#include "transcription/Transcriber.h"

#include <ModelData.h>
#include <juce_audio_formats/juce_audio_formats.h>

#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace
{

using namespace snotes;

struct Recording
{
    std::vector<float> samples;
    double sampleRate = 0.0;
};

/// All channels mixed to mono.
std::optional<Recording> readRecording(juce::AudioFormatManager& formats, const std::filesystem::path& file)
{
    const juce::File source{juce::String::fromUTF8(io::utf8FromPath(file).c_str())};
    const std::unique_ptr<juce::AudioFormatReader> reader{formats.createReaderFor(source)};
    if (reader == nullptr || reader->lengthInSamples > std::numeric_limits<int>::max())
        return std::nullopt;
    const auto length = static_cast<int>(reader->lengthInSamples);
    const auto channels = static_cast<int>(reader->numChannels);
    juce::AudioBuffer<float> buffer{channels, length};
    if (!reader->read(&buffer, 0, length, 0, true, true))
        return std::nullopt;
    Recording recording{.samples = std::vector<float>(static_cast<std::size_t>(length)),
                        .sampleRate = reader->sampleRate};
    for (int channel = 0; channel < channels; ++channel)
        juce::FloatVectorOperations::addWithMultiply(recording.samples.data(),
                                                     buffer.getReadPointer(channel),
                                                     1.0F / static_cast<float>(channels),
                                                     length);
    return recording;
}

void writeFile(const std::filesystem::path& file, const void* data, std::size_t size)
{
    std::ofstream stream{file, std::ios::binary};
    stream.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    if (!stream)
        throw std::runtime_error("Cannot write " + io::utf8FromPath(file));
}

/// <name>.notes.csv and <name>.mid in the output folder, or next to the recording.
void writeResults(const std::filesystem::path& file,
                  const std::string& outputFolder,
                  const std::vector<transcription::Note>& notes)
{
    const auto folder =
        outputFolder.empty() ? file.parent_path() : std::filesystem::path{io::pathFromUtf8(outputFolder)};
    const auto stem = folder / file.stem();
    const auto csv = transcription::notesToCsv(notes);
    const auto midi = transcription::notesToMidi(notes);
    writeFile(std::filesystem::path{stem} += ".notes.csv", csv.data(), csv.size());
    writeFile(std::filesystem::path{stem} += ".mid", midi.data(), midi.size());
}

bool transcribeFile(const std::filesystem::path& file,
                    const transcription::CommandLineOptions& options,
                    transcription::IPitchModel& model,
                    juce::AudioFormatManager& formats)
{
    const auto recording = readRecording(formats, file);
    if (!recording)
    {
        std::cerr << io::utf8FromPath(file) << ": no readable audio file\n";
        return false;
    }
    const auto started = std::chrono::steady_clock::now();
    const auto profile = model::findInstrumentProfile(options.profileId);
    if (!profile)
        return false;
    const auto notes = transcription::transcribe(recording->samples,
                                                 recording->sampleRate,
                                                 model,
                                                 transcription::settingsFor(*profile),
                                                 [](double) { return true; });
    if (!notes)
        return false;
    const std::chrono::duration<double> took = std::chrono::steady_clock::now() - started;

    writeResults(file, options.outputFolder, *notes);
    const double seconds = static_cast<double>(recording->samples.size()) / recording->sampleRate;
    std::cout << io::utf8FromPath(file) << ": " << notes->size() << " notes, " << std::fixed
              << std::setprecision(1) << seconds << " s audio in " << took.count() << " s\n";
    return true;
}

} // namespace

int main(int argc, char* argv[])
{
    const std::vector<std::string> arguments(argv + 1, argv + argc);
    const auto parsed = transcription::parseCommandLine(arguments);
    if (!parsed.options)
    {
        std::cerr << parsed.message;
        return parsed.message.starts_with("Usage") ? 0 : 2;
    }
    try
    {
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        transcription::OnnxPitchModel model{ModelData::basicpitch_onnx, ModelData::basicpitch_onnxSize};
        bool allRead = true;
        for (const auto& recording : parsed.options->recordings)
            allRead = transcribeFile(io::pathFromUtf8(recording), *parsed.options, model, formats) && allRead;
        return allRead ? 0 : 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
