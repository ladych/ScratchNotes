#include "ui/RhythmUpdate.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "model/Project.h"
#include "rhythm/RhythmAnalysis.h"
#include "rhythm/RhythmCsv.h"
#include "rhythm/Statistics.h"
#include "transcription/NoteCsv.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace snotes::ui
{
namespace
{

struct Analysed
{
    model::Take take;
    rhythm::RhythmTrack track;
};

/// The notes and onsets of the take, if both files can be read.
std::optional<rhythm::RhythmTrack> readTrack(const model::Take& take,
                                             const io::IFileSystem& fileSystem,
                                             const std::filesystem::path& folder,
                                             double sampleRate)
{
    const auto onsetsFile = take.onsetsFile();
    if (!take.hasNotes() || onsetsFile.empty())
        return std::nullopt;
    const auto notesText = fileSystem.readText(folder / io::pathFromUtf8(take.notesFile()));
    const auto onsetsText = fileSystem.readText(folder / io::pathFromUtf8(onsetsFile));
    if (!notesText || !onsetsText)
        return std::nullopt;
    auto notes = transcription::notesFromCsv(*notesText);
    auto onsets = rhythm::onsetsFromCsv(*onsetsText);
    if (!notes || !onsets)
        return std::nullopt;
    rhythm::RhythmTrack track;
    track.notes = std::move(*notes);
    track.onsets = std::move(*onsets);
    track.startSeconds = static_cast<double>(take.startSample() - take.latencyOffsetSamples()) / sampleRate;
    return track;
}

/// The active takes that can be analysed; the events of all takes are cleared.
std::vector<Analysed> analysedTakes(const model::Project& project,
                                    const io::IFileSystem& fileSystem,
                                    const std::filesystem::path& folder)
{
    std::vector<Analysed> takes;
    for (int index = 0; index < project.numVoices(); ++index)
    {
        const auto voice = project.voice(index);
        for (int take = 0; take < voice.numTakes(); ++take)
            voice.take(take).clearEvents();
        const auto active = voice.activeTake();
        if (!active)
            continue;
        if (auto track = readTrack(*active, fileSystem, folder, project.sampleRate()))
            takes.push_back({.take = *active, .track = std::move(*track)});
    }
    return takes;
}

void storeGrid(model::BeatGrid grid, const rhythm::BeatGrid& analysis, double sampleRate)
{
    std::vector<std::int64_t> beats;
    beats.reserve(analysis.beats.size());
    for (const double beat : analysis.beats)
        beats.push_back(std::llround(beat * sampleRate));
    std::vector<model::MeterReading> readings;
    readings.reserve(analysis.meter.ranking.size());
    for (const auto& candidate : analysis.meter.ranking)
        readings.push_back({.numerator = candidate.beatsPerBar,
                            .confidence = candidate.confidence,
                            .phase = candidate.phase});
    grid.setAnalysis(beats, readings);
}

} // namespace

std::string fileNextTo(const std::string& diFile, const std::string& suffix)
{
    auto file = io::pathFromUtf8(diFile);
    file.replace_extension("." + suffix);
    return io::genericUtf8FromPath(file);
}

RhythmUpdate
updateRhythm(juce::ValueTree project, io::IFileSystem& fileSystem, const std::filesystem::path& folder)
{
    const model::Project model{std::move(project), nullptr};
    auto takes = analysedTakes(model, fileSystem, folder);
    std::vector<rhythm::RhythmTrack> tracks;
    tracks.reserve(takes.size());
    for (const auto& analysed : takes)
        tracks.push_back(analysed.track);
    const auto grid = rhythm::commonBeatGrid(tracks, rhythm::BeatTrackerSettings{});
    if (!grid)
    {
        model.beatGrid().clear();
        return RhythmUpdate::noGrid;
    }
    const auto& analysis = *grid;
    storeGrid(model.beatGrid(), analysis, model.sampleRate());
    for (auto& analysed : takes)
    {
        const auto rhythm = rhythm::voiceRhythm(analysed.track, analysis);
        const auto file = fileNextTo(analysed.take.diFile(), "events.csv");
        if (!fileSystem.writeText(folder / io::pathFromUtf8(file), rhythm::eventsToCsv(rhythm)))
            return RhythmUpdate::writeFailed;
        analysed.take.setEvents(file, static_cast<int>(rhythm.quantization.events.size()));
    }
    return RhythmUpdate::updated;
}

std::string beatGridSummary(const juce::ValueTree& project)
{
    const model::Project model{project, nullptr};
    const auto grid = model.beatGrid();
    const auto beats = grid.beats();
    const auto readings = grid.meterReadings();
    if (beats.size() < 2 || readings.empty())
        return {};
    std::vector<double> tempo;
    for (std::size_t beat = 1; beat < beats.size(); ++beat)
        tempo.push_back(60.0 * model.sampleRate() / static_cast<double>(beats[beat] - beats[beat - 1]));
    const auto& best = readings.front();
    return i18n::tr("Beat grid: %1 BPM, %2/4 (%3 %)",
                    std::to_string(std::lround(rhythm::median(tempo))),
                    std::to_string(best.numerator),
                    std::to_string(std::lround(best.confidence * 100.0)));
}

std::vector<BeatLine> beatLines(const juce::ValueTree& project)
{
    const model::Project model{project, nullptr};
    const auto grid = model.beatGrid();
    const int meter = std::max(1, grid.chosenMeter());
    std::vector<BeatLine> lines;
    int index = 0;
    for (const auto beat : grid.beats())
    {
        lines.push_back({.seconds = static_cast<double>(beat) / model.sampleRate(),
                         .downbeat = grid.chosenMeter() > 0 &&
                                     ((index - grid.downbeatPhase()) % meter + meter) % meter == 0});
        ++index;
    }
    return lines;
}

} // namespace snotes::ui
