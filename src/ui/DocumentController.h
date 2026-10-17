#pragma once

#include "io/IProjectRepository.h"
#include "model/TreeChangeListener.h"
#include "ui/IDocumentView.h"

#include <juce_data_structures/juce_data_structures.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// What the DocumentController needs of the program around it.
struct DocumentEnvironment
{
    /// Where new projects get their folder, e.g. "Documents/Scratch Notes".
    std::filesystem::path projectsFolder;
    /// Local date and time for the names of new projects, e.g. "2026-10-17 09-40".
    std::function<std::string()> timestamp;
    /// Called with the folder of every project saved or opened, so the next start reopens it.
    std::function<void(const std::filesystem::path&)> rememberFolder;
};

/// Presenter for the menus "Datei" and "Bearbeiten": new, open, save, save as, close with the
/// question for unsaved changes (F-PJ-01 to F-PJ-03) and undo/redo (F-PJ-04).
///
/// A project is a folder with projekt.snotes and audio/. A new project is named after the date and time;
/// its folder in the projects folder is created when it is first saved, so "Speichern" never needs to
/// ask. "Speichern unter" asks for a new folder and copies the take audio there.
///
/// It works on one project tree that lives as long as the application, so listeners such as the
/// SnapshotPublisher stay attached: new and opened projects are copied into it, and their undo
/// history starts empty.
class DocumentController
{
public:
    DocumentController(juce::ValueTree project,
                       juce::UndoManager& undoManager,
                       io::IProjectRepository& repository,
                       IDocumentView& view,
                       DocumentEnvironment environment);

    /// Program start: reopens the last project, or starts a new one if there is none or it is gone.
    void start(const std::optional<std::filesystem::path>& lastFolder);

    void newProject();
    void open();
    void save(const std::function<void(bool saved)>& onDone = [](bool) {});
    void saveAs(std::function<void(bool saved)> onDone = [](bool) {});
    /// Calls onClose unless the user cancels the question for unsaved changes.
    void requestClose(std::function<void()> onClose);

    void undo();
    void redo();
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    [[nodiscard]] bool hasUnsavedChanges() const { return unsavedChanges_; }
    [[nodiscard]] const std::optional<std::filesystem::path>& currentFolder() const { return currentFolder_; }
    [[nodiscard]] std::string windowTitle() const;

private:
    void ifChangesSaved(std::function<void()> action);
    void startNewProject();
    /// Returns false if the project could not be loaded; `quietIfUnreadable` skips the message then.
    bool openFolder(const std::filesystem::path& folder, bool quietIfUnreadable);
    bool saveTo(const std::filesystem::path& folder);
    void replaceProject(const juce::ValueTree& newProject, std::optional<std::filesystem::path> folder);
    void setCurrentFolder(const std::filesystem::path& folder);
    void setUnsavedChanges(bool unsavedChanges);
    [[nodiscard]] std::string projectName() const;

    [[nodiscard]] static std::string loadErrorMessage(io::ProjectFileError error,
                                                      const std::string& folderName);
    [[nodiscard]] static std::string missingAudioMessage(const std::vector<io::MissingAudio>& missing);

    juce::ValueTree project_;
    juce::UndoManager& undoManager_;
    io::IProjectRepository& repository_;
    IDocumentView& view_;
    DocumentEnvironment environment_;
    std::optional<std::filesystem::path> currentFolder_;
    bool unsavedChanges_ = false;
    model::TreeChangeListener listener_;
};

} // namespace snotes::ui
