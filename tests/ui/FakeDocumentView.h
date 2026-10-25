#pragma once

#include "ui/IDocumentView.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// Answers every dialog immediately with preset answers and records what was shown.
class FakeDocumentView final : public IDocumentView
{
public:
    struct Message
    {
        std::string title;
        std::string text;
    };

    void setWindowTitle(const std::string& title) override { windowTitle = title; }

    void askToSaveChanges(const std::string& projectName,
                          std::function<void(SaveChangesChoice)> onChoice) override
    {
        askedToSave.push_back(projectName);
        onChoice(saveChangesAnswer);
    }

    void chooseProjectFile(const std::filesystem::path& startFolder, PathChosenCallback onChosen) override
    {
        openStartFolders.push_back(startFolder);
        onChosen(fileToOpen);
    }

    void chooseProjectFolder(const std::filesystem::path& suggestedFolder,
                             PathChosenCallback onChosen) override
    {
        suggestedFolders.push_back(suggestedFolder);
        onChosen(folderToSave);
    }

    void chooseExportFile(const std::filesystem::path& suggestedFile,
                          const std::string& pattern,
                          PathChosenCallback onChosen) override
    {
        suggestedExports.push_back(suggestedFile);
        exportPatterns.push_back(pattern);
        onChosen(fileToExport);
    }

    void showMessage(const std::string& title, const std::string& message) override
    {
        messages.push_back({title, message});
    }

    // Preset answers
    SaveChangesChoice saveChangesAnswer = SaveChangesChoice::cancel;
    std::optional<std::filesystem::path> fileToOpen;
    std::optional<std::filesystem::path> folderToSave;
    std::optional<std::filesystem::path> fileToExport;

    // Recorded calls
    std::string windowTitle;
    std::vector<std::string> askedToSave;
    std::vector<std::filesystem::path> openStartFolders;
    std::vector<std::filesystem::path> suggestedFolders;
    std::vector<std::filesystem::path> suggestedExports;
    std::vector<std::string> exportPatterns;
    std::vector<Message> messages;
};

} // namespace snotes::ui
