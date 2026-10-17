#pragma once

#include "io/IFileSystem.h"
#include "io/IProjectRepository.h"

namespace snotes::io
{

/// Project folders on disk. Saving writes projekt.snotes.tmp and renames it, so an interrupted save
/// leaves the last saved file intact (Q-10). A take whose audio file is missing on load is marked
/// audioMissing and reported (F-PJ-03).
class ProjectRepository final : public IProjectRepository
{
public:
    static constexpr const char* kProjectFileName = "projekt.snotes";
    static constexpr const char* kAudioFolderName = "audio";

    explicit ProjectRepository(IFileSystem& fileSystem);

    [[nodiscard]] LoadResult load(const std::filesystem::path& folder) override;
    [[nodiscard]] ProjectFileError save(const juce::ValueTree& project,
                                        const std::filesystem::path& folder) override;
    [[nodiscard]] ProjectFileError saveCopy(const juce::ValueTree& project,
                                            const std::filesystem::path& from,
                                            const std::filesystem::path& to) override;
    [[nodiscard]] std::filesystem::path freeFolder(const std::filesystem::path& wanted) const override;

private:
    IFileSystem& fileSystem_;
};

} // namespace snotes::io
