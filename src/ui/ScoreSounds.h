#pragma once

#include "io/IFileSystem.h"
#include "model/SnapshotBuilder.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// The voices of a standard MIDI file as notes in samples at sampleRate (F-NA-05): one entry per track
/// with notes, named after the track. None if the bytes are no MIDI file.
[[nodiscard]] std::vector<model::ScoreVoiceNotes> midiVoices(const std::string& bytes, double sampleRate);

/// The notes of the score as sound for the snapshots (model::ScoreNotesSource): the MIDI file of the score
/// (kMidiFile), which runs in sync with the recording. The file is read on every call, but only converted
/// when it changed.
class ScoreSounds
{
public:
    using FolderSource = std::function<std::optional<std::filesystem::path>()>;

    ScoreSounds(const io::IFileSystem& fileSystem, FolderSource folder);

    /// None without a project folder, a score or a readable MIDI file.
    [[nodiscard]] std::vector<model::ScoreVoiceNotes> voices(const juce::ValueTree& project);

private:
    const io::IFileSystem& fileSystem_;
    FolderSource folder_;
    std::string bytes_;
    double sampleRate_ = 0.0;
    std::vector<model::ScoreVoiceNotes> voices_;
};

} // namespace snotes::ui
