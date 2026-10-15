#include "app/AppComposition.h"
#include "app/Theme.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace snotes::app
{
namespace
{

class MainWindow final : public juce::DocumentWindow
{
public:
    MainWindow(const juce::String& name, std::unique_ptr<juce::Component> content, MainMenu& menu)
        : DocumentWindow(name,
                         juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                             juce::ResizableWindow::backgroundColourId),
                         DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar(true);
        setMenuBar(&menu);
        addKeyListener(menu.keyMappings());
        setContentOwned(content.release(), true);
        setResizable(true, true);
        centreWithSize(getWidth(), getHeight());
        setVisible(true);
    }

    ~MainWindow() override { setMenuBar(nullptr); }

    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    MainWindow(MainWindow&&) = delete;
    MainWindow& operator=(MainWindow&&) = delete;

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
};

class ScratchNotesApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise(const juce::String& /*commandLine*/) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel_);
        composition_ = std::make_unique<AppComposition>(lookAndFeel_, [] { quit(); });
        mainWindow_ = std::make_unique<MainWindow>(
            getApplicationName(), composition_->createMainComponent(), composition_->mainMenu());
        composition_->setDialogParent(mainWindow_.get());
    }

    void shutdown() override
    {
        mainWindow_.reset();
        composition_.reset();
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

    void systemRequestedQuit() override { quit(); }

private:
    theme::LookAndFeel lookAndFeel_;
    std::unique_ptr<AppComposition> composition_;
    std::unique_ptr<MainWindow> mainWindow_;
};

} // namespace
} // namespace snotes::app

START_JUCE_APPLICATION(snotes::app::ScratchNotesApplication)
