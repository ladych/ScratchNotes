#include "ui/DocumentController.h"

#include "i18n/Translate.h"
#include "io/Utf8Path.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <utility>

namespace snotes::ui
{
namespace
{

constexpr const char* kApplicationName = "Scratch Notes";
constexpr const char* kUnsavedMarker = "*";

} // namespace

DocumentController::DocumentController(juce::ValueTree project,
                                       juce::UndoManager& undoManager,
                                       io::IProjectRepository& repository,
                                       IDocumentView& view,
                                       DocumentEnvironment environment)
    : project_(std::move(project)), undoManager_(undoManager), repository_(repository), view_(view),
      environment_(std::move(environment)), listener_(project_, [this] { setUnsavedChanges(true); })
{
    view_.setWindowTitle(windowTitle());
}

void DocumentController::start(const std::optional<std::filesystem::path>& lastFolder)
{
    if (!lastFolder || !openFolder(*lastFolder, true))
        startNewProject();
}

void DocumentController::newProject()
{
    ifChangesSaved([this] { startNewProject(); });
}

void DocumentController::open()
{
    ifChangesSaved(
        [this]
        {
            view_.chooseProjectFile(currentFolder_.value_or(environment_.projectsFolder),
                                    [this](const std::optional<std::filesystem::path>& file)
                                    {
                                        if (file)
                                            openFolder(file->parent_path(), false);
                                    });
        });
}

void DocumentController::save(const std::function<void(bool saved)>& onDone)
{
    if (currentFolder_)
    {
        onDone(saveTo(*currentFolder_));
        return;
    }
    onDone(saveTo(repository_.freeFolder(environment_.projectsFolder / io::pathFromUtf8(projectName()))));
}

void DocumentController::saveAs(std::function<void(bool saved)> onDone)
{
    view_.chooseProjectFolder(
        environment_.projectsFolder / io::pathFromUtf8(projectName()),
        [this, onDone = std::move(onDone)](const std::optional<std::filesystem::path>& folder)
        {
            if (!folder)
            {
                onDone(false);
                return;
            }
            // The project takes the name of its new folder; the old folder keeps its own file.
            model::Project{project_, nullptr}.setName(io::utf8FromPath(folder->filename()));
            const auto error = currentFolder_ ? repository_.saveCopy(project_, *currentFolder_, *folder)
                                              : repository_.save(project_, *folder);
            if (error != io::ProjectFileError::none)
            {
                view_.showMessage(
                    i18n::tr("Saving failed"),
                    i18n::tr("The project could not be saved in \"%1\".", io::utf8FromPath(*folder)));
                onDone(false);
                return;
            }
            setCurrentFolder(*folder);
            onDone(true);
        });
}

void DocumentController::requestClose(std::function<void()> onClose)
{
    ifChangesSaved(std::move(onClose));
}

void DocumentController::undo()
{
    undoManager_.undo();
}

void DocumentController::redo()
{
    undoManager_.redo();
}

bool DocumentController::canUndo() const
{
    return undoManager_.canUndo();
}

bool DocumentController::canRedo() const
{
    return undoManager_.canRedo();
}

std::string DocumentController::windowTitle() const
{
    return (unsavedChanges_ ? kUnsavedMarker : "") + projectName() + " – " + kApplicationName;
}

void DocumentController::ifChangesSaved(std::function<void()> action)
{
    if (!unsavedChanges_)
    {
        action();
        return;
    }
    view_.askToSaveChanges(projectName(),
                           [this, action = std::move(action)](SaveChangesChoice choice)
                           {
                               if (choice == SaveChangesChoice::discard)
                                   action();
                               else if (choice == SaveChangesChoice::save)
                                   save(
                                       [action](bool saved)
                                       {
                                           if (saved)
                                               action();
                                       });
                           });
}

void DocumentController::startNewProject()
{
    replaceProject(model::ProjectFactory::createDefault(i18n::tr("Notes %1", environment_.timestamp())),
                   std::nullopt);
}

bool DocumentController::openFolder(const std::filesystem::path& folder, bool quietIfUnreadable)
{
    const auto result = repository_.load(folder);
    if (result.error != io::ProjectFileError::none)
    {
        if (!quietIfUnreadable || result.error != io::ProjectFileError::unreadable)
            view_.showMessage(i18n::tr("Opening failed"),
                              loadErrorMessage(result.error, io::utf8FromPath(folder.filename())));
        return false;
    }
    replaceProject(result.project, folder);
    environment_.rememberFolder(folder);
    if (!result.missingAudio.empty())
        view_.showMessage(i18n::tr("Missing audio files"), missingAudioMessage(result.missingAudio));
    return true;
}

bool DocumentController::saveTo(const std::filesystem::path& folder)
{
    if (repository_.save(project_, folder) != io::ProjectFileError::none)
    {
        view_.showMessage(i18n::tr("Saving failed"),
                          i18n::tr("The project could not be saved in \"%1\".", io::utf8FromPath(folder)));
        return false;
    }
    setCurrentFolder(folder);
    return true;
}

void DocumentController::replaceProject(const juce::ValueTree& newProject,
                                        std::optional<std::filesystem::path> folder)
{
    project_.copyPropertiesAndChildrenFrom(newProject, nullptr);
    undoManager_.clearUndoHistory();
    currentFolder_ = std::move(folder);
    setUnsavedChanges(false);
}

void DocumentController::setCurrentFolder(const std::filesystem::path& folder)
{
    currentFolder_ = folder;
    environment_.rememberFolder(folder);
    setUnsavedChanges(false);
}

void DocumentController::setUnsavedChanges(bool unsavedChanges)
{
    unsavedChanges_ = unsavedChanges;
    view_.setWindowTitle(windowTitle());
}

std::string DocumentController::projectName() const
{
    if (currentFolder_)
        return io::utf8FromPath(currentFolder_->filename());
    return model::Project{project_, nullptr}.name();
}

std::string DocumentController::loadErrorMessage(io::ProjectFileError error, const std::string& folderName)
{
    switch (error)
    {
    case io::ProjectFileError::unreadable:
        return i18n::tr("\"%1\" contains no readable projekt.snotes.", folderName);
    case io::ProjectFileError::newerFormatVersion:
        return i18n::tr("\"%1\" was saved with a newer version of Scratch Notes. Please update the program.",
                        folderName);
    default:
        return i18n::tr("The projekt.snotes in \"%1\" is not a Scratch Notes project.", folderName);
    }
}

std::string DocumentController::missingAudioMessage(const std::vector<io::MissingAudio>& missing)
{
    std::string message = i18n::tr("These audio files were not found. Their takes stay in the project but "
                                   "are silent until the files are back in the project folder:\n\n");
    for (const auto& audio : missing)
        message += audio.voiceName + ": " + audio.file + "\n";
    return message;
}

} // namespace snotes::ui
