#include "ui/ScoreExport.h"

#include "i18n/Translate.h"
#include "io/MockFileSystem.h"
#include "io/Utf8Path.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "ui/FakeDocumentView.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <optional>

namespace snotes::ui
{
namespace
{

namespace fs = std::filesystem;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

/// A project "Probe: 1/2" whose notes were detected, so its folder holds the score.
class ScoreExportTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        project.setName("Probe: 1/2");
        project.beatGrid().setScoreFile("score.musicxml");
        ON_CALL(fileSystem, exists(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, copyFile(_, _)).WillByDefault(Return(true));
    }

    fs::path folder = fs::path{"projects"} / "Probe";
    std::optional<fs::path> currentFolder = folder;
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, nullptr};
    NiceMock<io::MockFileSystem> fileSystem;
    FakeDocumentView view;
    ScoreExport scoreExport{tree, fileSystem, view, [this] { return currentFolder; }};
};

TEST_F(ScoreExportTest, FEX01_CopiesTheMidiFileOfTheScoreWhereTheUserWants)
{
    view.fileToExport = fs::path{"export"} / "Riff.mid";
    EXPECT_CALL(fileSystem, copyFile(folder / "score.mid", fs::path{"export"} / "Riff.mid"))
        .WillOnce(Return(true));

    scoreExport.exportAs(ExportFormat::midi);

    ASSERT_EQ(view.suggestedExports.size(), 1U);
    EXPECT_EQ(view.suggestedExports.front(), folder / "Probe_ 1_2.mid");
    EXPECT_EQ(view.exportPatterns.front(), "*.mid");
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(ScoreExportTest, FEX02_CopiesTheMusicXmlAndAddsTheExtension)
{
    view.fileToExport = fs::path{"export"} / "Riff";
    EXPECT_CALL(fileSystem, copyFile(folder / "score.musicxml", fs::path{"export"} / "Riff.musicxml"))
        .WillOnce(Return(true));

    scoreExport.exportAs(ExportFormat::musicXml);

    EXPECT_EQ(view.exportPatterns.front(), "*.musicxml");
}

TEST_F(ScoreExportTest, FEX01_NothingHappensWhenTheUserCancels)
{
    EXPECT_CALL(fileSystem, copyFile(_, _)).Times(0);

    scoreExport.exportAs(ExportFormat::midi);

    EXPECT_EQ(view.suggestedExports.size(), 1U);
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(ScoreExportTest, FEX01_ReportsAFileThatCannotBeWritten)
{
    view.fileToExport = fs::path{"export"} / "Riff.mid";
    EXPECT_CALL(fileSystem, copyFile(_, _)).WillOnce(Return(false));

    scoreExport.exportAs(ExportFormat::midi);

    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(
        view.messages.front().text,
        i18n::tr("The score could not be written to %1.", io::utf8FromPath(fs::path{"export"} / "Riff.mid")));
}

TEST_F(ScoreExportTest, FEX01_AProjectWithoutANameSuggestsScore)
{
    project.setName("");

    scoreExport.exportAs(ExportFormat::midi);

    ASSERT_EQ(view.suggestedExports.size(), 1U);
    EXPECT_EQ(view.suggestedExports.front(), folder / "score.mid");
}

TEST_F(ScoreExportTest, FEX01_WithoutDetectedNotesThereIsNothingToExport)
{
    project.beatGrid().setScoreFile("");

    scoreExport.exportAs(ExportFormat::midi);

    EXPECT_TRUE(view.suggestedExports.empty());
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages.front().text, i18n::tr("There is no score yet. Detect the notes first."));
}

TEST_F(ScoreExportTest, FEX01_AnUnsavedProjectHasNoScore)
{
    currentFolder.reset();

    scoreExport.exportAs(ExportFormat::musicXml);

    EXPECT_TRUE(view.suggestedExports.empty());
    EXPECT_EQ(view.messages.size(), 1U);
}

TEST_F(ScoreExportTest, FEX01_AScoreFileThatIsGoneCannotBeExported)
{
    EXPECT_CALL(fileSystem, exists(folder / "score.mid")).WillOnce(Return(false));

    scoreExport.exportAs(ExportFormat::midi);

    EXPECT_TRUE(view.suggestedExports.empty());
    EXPECT_EQ(view.messages.size(), 1U);
}

} // namespace
} // namespace snotes::ui
