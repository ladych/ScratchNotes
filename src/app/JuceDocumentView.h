#pragma once

#include "ui/IDocumentView.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace snotes::app
{

/// Humble object (E-03): window title, message boxes and file choosers for the DocumentController,
/// all asynchronous.
class JuceDocumentView final : public ui::IDocumentView
{
public:
    using TitleSink = std::function<void(const juce::String&)>;

    /// The window is created after the controller, so the title is buffered until it exists.
    void setTitleSink(TitleSink sink);

    void setWindowTitle(const std::string& title) override;
    void askToSaveChanges(const std::string& projectName,
                          std::function<void(ui::SaveChangesChoice)> onChoice) override;
    void chooseProjectFile(const std::filesystem::path& startFolder,
                           ui::PathChosenCallback onChosen) override;
    void chooseProjectFolder(const std::filesystem::path& suggestedFolder,
                             ui::PathChosenCallback onChosen) override;
    void chooseExportFile(const std::filesystem::path& suggestedFile,
                          const std::string& pattern,
                          ui::PathChosenCallback onChosen) override;
    void showMessage(const std::string& title, const std::string& message) override;

private:
    void launchChooser(const juce::String& title,
                       const juce::File& start,
                       const juce::String& pattern,
                       int flags,
                       ui::PathChosenCallback onChosen);

    TitleSink titleSink_;
    juce::String title_;
    std::unique_ptr<juce::FileChooser> chooser_;
};

} // namespace snotes::app
