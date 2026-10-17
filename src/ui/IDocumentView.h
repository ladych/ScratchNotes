#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

namespace snotes::ui
{

enum class SaveChangesChoice : std::uint8_t
{
    save,
    discard,
    cancel
};

using PathChosenCallback = std::function<void(std::optional<std::filesystem::path>)>;

/// Window title and dialogs the DocumentController needs. Dialogs are asynchronous: each reports
/// the user's answer through its callback, so the JUCE implementation needs no modal loops.
class IDocumentView
{
public:
    virtual ~IDocumentView() = default;

    virtual void setWindowTitle(const std::string& title) = 0;
    virtual void askToSaveChanges(const std::string& projectName,
                                  std::function<void(SaveChangesChoice)> onChoice) = 0;
    /// Asks for a projekt.snotes file, starting in `startFolder`. Reports std::nullopt if the user cancels.
    virtual void chooseProjectFile(const std::filesystem::path& startFolder, PathChosenCallback onChosen) = 0;
    /// Asks for the folder to save the project in, suggesting `suggestedFolder`. Reports std::nullopt if
    /// the user cancels.
    virtual void chooseProjectFolder(const std::filesystem::path& suggestedFolder,
                                     PathChosenCallback onChosen) = 0;
    virtual void showMessage(const std::string& title, const std::string& message) = 0;

protected:
    IDocumentView() = default;
    IDocumentView(const IDocumentView&) = default;
    IDocumentView(IDocumentView&&) = default;
    IDocumentView& operator=(const IDocumentView&) = default;
    IDocumentView& operator=(IDocumentView&&) = default;
};

} // namespace snotes::ui
