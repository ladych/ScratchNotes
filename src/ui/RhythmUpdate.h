#pragma once

#include "io/IFileSystem.h"

#include <cstdint>
#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <string>
#include <vector>

namespace snotes::ui
{

enum class RhythmUpdate : std::uint8_t
{
    /// The project has a beat grid and the active takes their events.
    updated,
    /// No transcribed take with beats; the grid and the events are cleared.
    noGrid,
    /// An events file could not be written.
    writeFailed,
};

/// The score of all voices in the project folder (F-NA-02).
inline constexpr const char* kScoreFile = "score.musicxml";
/// The same score as MIDI file in sync with the recording (F-EX-01).
inline constexpr const char* kMidiFile = "score.mid";

/// Computes the common beat grid from the active takes with notes and onsets (F-TX-04 to 06) and the events
/// of each of them (F-TX-07, F-TX-09) with the tablature for the voice's profile (F-TB-01, F-TB-02), written
/// next to the DI file as "<DI file name>.events.csv".
/// Events of the other takes are forgotten. Then the score of all voices in the chosen meter goes to
/// kScoreFile (F-NA-01, F-NA-02) with the median tempo (F-EX-02) and to kMidiFile (F-EX-01).
[[nodiscard]] RhythmUpdate
updateRhythm(juce::ValueTree project, io::IFileSystem& fileSystem, const std::filesystem::path& folder);

/// E.g. "Beat grid: 86 BPM, 4/4 (69 %)" with the median tempo; empty without a grid.
[[nodiscard]] std::string beatGridSummary(const juce::ValueTree& project);

/// A beat of the grid in the track view (F-NA-03).
struct BeatLine
{
    double seconds = 0.0;
    /// First beat of a bar in the chosen meter.
    bool downbeat = false;

    bool operator==(const BeatLine&) const = default;
};

[[nodiscard]] std::vector<BeatLine> beatLines(const juce::ValueTree& project);

/// "audio/Bass Take 1 DI.wav" becomes "audio/Bass Take 1 DI.<suffix>", e.g. "events.csv".
[[nodiscard]] std::string fileNextTo(const std::string& diFile, const std::string& suffix);

} // namespace snotes::ui
