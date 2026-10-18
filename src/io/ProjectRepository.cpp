#include "io/ProjectRepository.h"

#include "io/Utf8Path.h"
#include "model/ModelIds.h"
#include "model/Project.h"

#include <string>
#include <utility>
#include <vector>

namespace snotes::io
{
namespace ids = model::ids;

namespace
{

struct TakeFile
{
    juce::ValueTree voice;
    juce::ValueTree take;
    /// Relative to the project folder.
    std::string file;
};

/// The audio files of every take.
std::vector<TakeFile> takeFiles(const juce::ValueTree& project)
{
    std::vector<TakeFile> files;
    for (const auto& voice : project.getChildWithName(ids::voices))
        for (const auto& take : voice.getChildWithName(ids::takes))
            for (const auto* property : {&ids::diFile, &ids::wetFile})
            {
                auto file = take[*property].toString().toStdString();
                if (!file.empty())
                    files.push_back({voice, take, std::move(file)});
            }
    return files;
}

/// The takes with a transcription (F-TX-01).
std::vector<juce::ValueTree> transcribedTakes(const juce::ValueTree& project)
{
    std::vector<juce::ValueTree> takes;
    for (const auto& voice : project.getChildWithName(ids::voices))
        for (const auto& take : voice.getChildWithName(ids::takes))
            if (take.hasProperty(ids::notesFile))
                takes.push_back(take);
    return takes;
}

std::filesystem::path notesPath(const std::filesystem::path& folder, const juce::ValueTree& take)
{
    return folder / pathFromUtf8(take[ids::notesFile].toString().toStdString());
}

} // namespace

ProjectRepository::ProjectRepository(IFileSystem& fileSystem) : fileSystem_(fileSystem) {}

LoadResult ProjectRepository::load(const std::filesystem::path& folder)
{
    const auto text = fileSystem_.readText(folder / kProjectFileName);
    if (!text)
        return {{}, ProjectFileError::unreadable, {}};

    auto parsed = ProjectSerializer::fromXml(*text);
    if (parsed.error != ProjectFileError::none)
        return {{}, parsed.error, {}};

    std::vector<MissingAudio> missing;
    for (auto& takeFile : takeFiles(parsed.project))
    {
        const auto path = folder / pathFromUtf8(takeFile.file);
        if (fileSystem_.exists(path))
            continue;
        takeFile.take.setProperty(ids::audioMissing, true, nullptr);
        missing.push_back({takeFile.voice[ids::name].toString().toStdString(), takeFile.file});
    }
    // A lost transcription is not reported; the take is transcribed again.
    for (const auto& take : transcribedTakes(parsed.project))
        if (!fileSystem_.exists(notesPath(folder, take)))
            model::Take{take, nullptr}.clearNotes();
    return {parsed.project, ProjectFileError::none, std::move(missing)};
}

ProjectFileError ProjectRepository::save(const juce::ValueTree& project, const std::filesystem::path& folder)
{
    const auto file = folder / kProjectFileName;
    auto tempFile = file;
    tempFile += ".tmp";
    const auto audioFolder = folder / kAudioFolderName;
    if (!fileSystem_.createDirectories(audioFolder))
        return ProjectFileError::writeFailed;
    const auto text = ProjectSerializer::toXml(project);
    if (!fileSystem_.writeText(tempFile, text) || !fileSystem_.rename(tempFile, file))
        return ProjectFileError::writeFailed;
    return ProjectFileError::none;
}

ProjectFileError ProjectRepository::saveCopy(const juce::ValueTree& project,
                                             const std::filesystem::path& from,
                                             const std::filesystem::path& to)
{
    const auto audioFolder = to / kAudioFolderName;
    if (!fileSystem_.createDirectories(audioFolder))
        return ProjectFileError::writeFailed;
    for (const auto& takeFile : takeFiles(project))
    {
        // A missing file stays missing in the copy; it was reported when the project was loaded.
        if (static_cast<bool>(takeFile.take[ids::audioMissing]))
            continue;
        const auto relative = pathFromUtf8(takeFile.file);
        const auto source = from / relative;
        const auto target = to / relative;
        if (!fileSystem_.copyFile(source, target))
            return ProjectFileError::writeFailed;
    }
    for (const auto& take : transcribedTakes(project))
    {
        const auto source = notesPath(from, take);
        if (!fileSystem_.exists(source))
            continue;
        if (!fileSystem_.copyFile(source, notesPath(to, take)))
            return ProjectFileError::writeFailed;
    }
    return save(project, to);
}

std::filesystem::path ProjectRepository::freeFolder(const std::filesystem::path& wanted) const
{
    auto candidate = wanted;
    for (int number = 2; fileSystem_.exists(candidate); ++number)
    {
        candidate = wanted;
        candidate += " (" + std::to_string(number) + ")";
    }
    return candidate;
}

} // namespace snotes::io
