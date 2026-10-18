// scratchnotes-transcribe (Pflichtenheft 3, tools/): detects the notes of DI recordings without the program,
// for checking the note detection against the Python prototype and for batch runs over old recordings.
// Then the rhythm: one beat grid for all recordings (or one per recording with --separate), the meter and
// the quantized events of each voice.

#include "io/Utf8Path.h"
#include "model/InstrumentProfile.h"
#include "rhythm/OnsetStrength.h"
#include "rhythm/RhythmAnalysis.h"
#include "rhythm/RhythmCsv.h"
#include "transcription/CommandLine.h"
#include "transcription/NoteCsv.h"
#include "transcription/NoteMidi.h"
#include "transcription/OnnxPitchModel.h"
#include "transcription/Resampler.h"
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

std::filesystem::path outputStem(const std::filesystem::path& file, const std::string& outputFolder)
{
    const auto folder =
        outputFolder.empty() ? file.parent_path() : std::filesystem::path{io::pathFromUtf8(outputFolder)};
    return folder / file.stem();
}

void writeText(const std::filesystem::path& stem, const char* extension, const std::string& text)
{
    writeFile(std::filesystem::path{stem} += extension, text.data(), text.size());
}

/// <name>.notes.csv and <name>.mid in the output folder, or next to the recording.
void writeNotes(const std::filesystem::path& stem, const std::vector<transcription::Note>& notes)
{
    const auto midi = transcription::notesToMidi(notes);
    writeText(stem, ".notes.csv", transcription::notesToCsv(notes));
    writeFile(std::filesystem::path{stem} += ".mid", midi.data(), midi.size());
}

struct Transcribed
{
    std::filesystem::path file;
    rhythm::RhythmTrack track;
};

std::optional<Transcribed> transcribeFile(const std::filesystem::path& file,
                                          const transcription::CommandLineOptions& options,
                                          transcription::IPitchModel& model,
                                          juce::AudioFormatManager& formats)
{
    const auto recording = readRecording(formats, file);
    if (!recording)
    {
        std::cerr << io::utf8FromPath(file) << ": no readable audio file\n";
        return std::nullopt;
    }
    const auto started = std::chrono::steady_clock::now();
    const auto profile = model::findInstrumentProfile(options.profileId);
    if (!profile)
        return std::nullopt;
    auto notes = transcription::transcribe(recording->samples,
                                           recording->sampleRate,
                                           model,
                                           transcription::settingsFor(*profile),
                                           [](double) { return true; });
    if (!notes)
        return std::nullopt;
    const auto onsets = rhythm::measureOnsets(
        transcription::resample(recording->samples, recording->sampleRate, rhythm::kAnalysisRate));
    const std::chrono::duration<double> took = std::chrono::steady_clock::now() - started;

    writeNotes(outputStem(file, options.outputFolder), *notes);
    const double seconds = static_cast<double>(recording->samples.size()) / recording->sampleRate;
    std::cout << io::utf8FromPath(file) << ": " << notes->size() << " notes, " << std::fixed
              << std::setprecision(1) << seconds << " s audio in " << took.count() << " s\n";
    return Transcribed{.file = file,
                       .track = {.notes = std::move(*notes), .onsets = onsets, .startSeconds = 0.0}};
}

/// The grid of the voices to <first name>.beats.csv and .meter.csv, their events to <name>.events.csv.
void writeRhythm(const std::vector<Transcribed>& voices, const std::string& outputFolder)
{
    const auto started = std::chrono::steady_clock::now();
    std::vector<rhythm::RhythmTrack> tracks;
    tracks.reserve(voices.size());
    for (const auto& voice : voices)
        tracks.push_back(voice.track);
    const auto grid = rhythm::commonBeatGrid(tracks, rhythm::BeatTrackerSettings{});
    const auto first = io::utf8FromPath(voices.front().file);
    if (!grid)
    {
        std::cout << first << ": no beats found\n";
        return;
    }
    const auto& analysis = *grid;
    const auto stem = outputStem(voices.front().file, outputFolder);
    writeText(stem, ".beats.csv", rhythm::beatsToCsv(analysis));
    writeText(stem, ".meter.csv", rhythm::meterToCsv(analysis));
    const auto& meter = analysis.meter.ranking.front();
    std::cout << first << ": " << analysis.beats.size() << " beats at " << std::fixed << std::setprecision(1)
              << analysis.tempoCandidates.front().bpm << " BPM, meter " << meter.beatsPerBar << "/4 ("
              << std::setprecision(0) << 100.0 * meter.confidence << " %)\n";
    for (const auto& voice : voices)
    {
        const auto rhythm = rhythm::voiceRhythm(voice.track, analysis);
        writeText(outputStem(voice.file, outputFolder), ".events.csv", rhythm::eventsToCsv(rhythm));
        std::cout << io::utf8FromPath(voice.file) << ": " << rhythm.quantization.events.size() << " events, "
                  << (rhythm.style.style == rhythm::Style::riff ? "riff" : "melody") << '\n';
    }
    const std::chrono::duration<double> took = std::chrono::steady_clock::now() - started;
    std::cout << "Rhythm in " << std::setprecision(2) << took.count() << " s\n";
}

/// Transcribes every recording; false if one could not be read.
bool transcribeAll(const transcription::CommandLineOptions& options,
                   transcription::OnnxPitchModel& model,
                   juce::AudioFormatManager& formats)
{
    bool allRead = true;
    std::vector<Transcribed> voices;
    for (const auto& recording : options.recordings)
    {
        auto transcribed = transcribeFile(io::pathFromUtf8(recording), options, model, formats);
        allRead = transcribed.has_value() && allRead;
        if (transcribed && options.separateGrids)
            writeRhythm({*transcribed}, options.outputFolder);
        else if (transcribed)
            voices.push_back(std::move(*transcribed));
    }
    if (!voices.empty())
        writeRhythm(voices, options.outputFolder);
    return allRead;
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
        return transcribeAll(*parsed.options, model, formats) ? 0 : 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
