#pragma once

#include "io/IFileSystem.h"
#include "ui/IDocumentView.h"

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace snotes::ui
{

/// The formats of the menu "Datei": "Als MIDI exportieren…" (F-EX-01) and "Als MusicXML exportieren…"
/// (F-EX-02).
enum class ExportFormat : std::uint8_t
{
    midi,
    musicXml,
};

/// Exports the score written by the last note detection (ui::kMidiFile, ui::kScoreFile) to a file the user
/// chooses, suggested as "<project name>.mid" or ".musicxml" in the project folder.
class ScoreExport
{
public:
    using FolderProvider = std::function<std::optional<std::filesystem::path>()>;

    ScoreExport(juce::ValueTree project,
                io::IFileSystem& fileSystem,
                IDocumentView& view,
                FolderProvider folder);

    /// Tells the user to detect the notes first if there is no score yet.
    void exportAs(ExportFormat format);

private:
    /// Copies source_ to target, which gets extension_ if the user left it out.
    void copyTo(std::filesystem::path target);

    juce::ValueTree project_;
    io::IFileSystem& fileSystem_;
    IDocumentView& view_;
    FolderProvider folder_;
    std::filesystem::path source_;
    std::string extension_;
};

} // namespace snotes::ui
