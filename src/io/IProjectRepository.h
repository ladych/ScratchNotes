#pragma once

#include "io/ProjectSerializer.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <string>
#include <vector>

namespace snotes::io
{

struct MissingAudio
{
    std::string voiceName;
    /// Relative to the project folder, as in projekt.snotes.
    std::string file;
};

struct LoadResult
{
    juce::ValueTree project;
    ProjectFileError error = ProjectFileError::none;
    std::vector<MissingAudio> missingAudio;
};

/// Reads and writes project folders: projekt.snotes and the take audio in audio/ (F-PJ-02).
class IProjectRepository
{
public:
    virtual ~IProjectRepository() = default;

    [[nodiscard]] virtual LoadResult load(const std::filesystem::path& folder) = 0;
    [[nodiscard]] virtual ProjectFileError save(const juce::ValueTree& project,
                                                const std::filesystem::path& folder) = 0;
    /// Save as: copies the take audio from `from` into `to`, then saves the project there.
    [[nodiscard]] virtual ProjectFileError saveCopy(const juce::ValueTree& project,
                                                    const std::filesystem::path& from,
                                                    const std::filesystem::path& to) = 0;
    /// The state after a crash (F-AU-06, Q-09): written next to projekt.snotes while there are unsaved
    /// changes, read like load(), deleted once the project is saved or the changes are discarded.
    [[nodiscard]] virtual ProjectFileError saveRecovery(const juce::ValueTree& project,
                                                        const std::filesystem::path& folder) = 0;
    [[nodiscard]] virtual bool hasRecovery(const std::filesystem::path& folder) const = 0;
    [[nodiscard]] virtual LoadResult loadRecovery(const std::filesystem::path& folder) = 0;
    virtual void removeRecovery(const std::filesystem::path& folder) = 0;
    /// `wanted` if nothing exists there yet, otherwise "wanted (2)", "wanted (3)" ...
    [[nodiscard]] virtual std::filesystem::path freeFolder(const std::filesystem::path& wanted) const = 0;

protected:
    IProjectRepository() = default;
    IProjectRepository(const IProjectRepository&) = default;
    IProjectRepository(IProjectRepository&&) = default;
    IProjectRepository& operator=(const IProjectRepository&) = default;
    IProjectRepository& operator=(IProjectRepository&&) = default;
};

} // namespace snotes::io
