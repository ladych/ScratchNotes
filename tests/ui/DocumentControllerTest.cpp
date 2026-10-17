#include "ui/DocumentController.h"

#include "io/MockProjectRepository.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "ui/FakeDocumentView.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace snotes::ui
{
namespace
{

namespace fs = std::filesystem;
using io::LoadResult;
using io::ProjectFileError;
using model::Project;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnArg;

class DocumentControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(repository, save(_, _)).WillByDefault(Return(ProjectFileError::none));
        ON_CALL(repository, saveCopy(_, _, _)).WillByDefault(Return(ProjectFileError::none));
        ON_CALL(repository, freeFolder(_)).WillByDefault(ReturnArg<0>());
        controller.start(std::nullopt);
    }

    static LoadResult loadableProject(const std::string& name)
    {
        return {model::ProjectFactory::createDefault(name), ProjectFileError::none, {}};
    }

    void makeChange()
    {
        undoManager.beginNewTransaction();
        project.addVoice(
            {.id = "v" + std::to_string(project.numVoices()), .name = "Gitarre", .profileId = "guitar6-e"});
    }

    void saveAs(const fs::path& folder)
    {
        view.folderToSave = folder;
        controller.saveAs();
    }

    fs::path projectsFolder = fs::path{"Dokumente"} / "Scratch Notes";
    fs::path rehearsal = fs::path{"Band"} / "Probe";
    std::vector<fs::path> remembered;

    juce::UndoManager undoManager;
    juce::ValueTree tree = model::ProjectFactory::createDefault("leer");
    Project project{tree, &undoManager};
    NiceMock<io::MockProjectRepository> repository;
    FakeDocumentView view;
    DocumentController controller{
        tree,
        undoManager,
        repository,
        view,
        {.projectsFolder = projectsFolder,
         .timestamp = [] { return std::string{"2026-10-17 09-40"}; },
         .rememberFolder = [this](const fs::path& folder) { remembered.push_back(folder); }}};
};

// ----- Start, window title and unsaved changes ----------------------------------------------------

TEST_F(DocumentControllerTest, FPJ01_StartsANewProjectNamedAfterDateAndTime)
{
    EXPECT_EQ(project.name(), "Notizen 2026-10-17 09-40");
    EXPECT_EQ(view.windowTitle, "Notizen 2026-10-17 09-40 – Scratch Notes");
    EXPECT_EQ(controller.windowTitle(), view.windowTitle);
    EXPECT_FALSE(controller.currentFolder().has_value());
    EXPECT_FALSE(controller.hasUnsavedChanges());
}

TEST_F(DocumentControllerTest, FPJ02_StartReopensTheLastProject)
{
    EXPECT_CALL(repository, load(rehearsal)).WillOnce(Return(loadableProject("Probe")));

    controller.start(rehearsal);

    EXPECT_EQ(controller.currentFolder(), rehearsal);
    EXPECT_EQ(view.windowTitle, "Probe – Scratch Notes");
    EXPECT_THAT(remembered, ElementsAre(rehearsal));
}

TEST_F(DocumentControllerTest, FPJ02_StartWithAVanishedProjectStartsANewOneQuietly)
{
    EXPECT_CALL(repository, load(_)).WillOnce(Return(LoadResult{{}, ProjectFileError::unreadable, {}}));
    project.setName("anders");

    controller.start(rehearsal);

    EXPECT_EQ(project.name(), "Notizen 2026-10-17 09-40");
    EXPECT_TRUE(view.messages.empty());
}

TEST_F(DocumentControllerTest, FPJ02_StartWithABrokenProjectSaysSo)
{
    EXPECT_CALL(repository, load(_)).WillOnce(Return(LoadResult{{}, ProjectFileError::notAProject, {}}));

    controller.start(rehearsal);

    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Öffnen fehlgeschlagen");
    EXPECT_FALSE(controller.currentFolder().has_value());
}

TEST_F(DocumentControllerTest, FPJ04_TitleMarksUnsavedChanges)
{
    makeChange();

    EXPECT_TRUE(controller.hasUnsavedChanges());
    EXPECT_EQ(view.windowTitle, "*Notizen 2026-10-17 09-40 – Scratch Notes");
}

// ----- Save and save as ---------------------------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ01_FirstSaveCreatesTheFolderInTheProjectsFolder)
{
    makeChange();
    const auto expected = projectsFolder / "Notizen 2026-10-17 09-40 (2)";
    EXPECT_CALL(repository, freeFolder(projectsFolder / "Notizen 2026-10-17 09-40"))
        .WillOnce(Return(expected));
    EXPECT_CALL(repository, save(tree, expected)).WillOnce(Return(ProjectFileError::none));
    bool saved = false;

    controller.save([&saved](bool result) { saved = result; });

    EXPECT_TRUE(saved);
    EXPECT_EQ(controller.currentFolder(), expected);
    EXPECT_FALSE(controller.hasUnsavedChanges());
    EXPECT_THAT(remembered, ElementsAre(expected));
    EXPECT_TRUE(view.suggestedFolders.empty());
}

TEST_F(DocumentControllerTest, FPJ02_SaveWritesTheCurrentFolderAgain)
{
    saveAs(rehearsal);
    makeChange();
    EXPECT_CALL(repository, save(tree, rehearsal)).WillOnce(Return(ProjectFileError::none));
    EXPECT_CALL(repository, freeFolder(_)).Times(0);

    controller.save();

    EXPECT_FALSE(controller.hasUnsavedChanges());
}

TEST_F(DocumentControllerTest, FPJ02_FailedSaveKeepsTheChangesAndSaysSo)
{
    makeChange();
    EXPECT_CALL(repository, save(_, _)).WillOnce(Return(ProjectFileError::writeFailed));
    bool saved = true;

    controller.save([&saved](bool result) { saved = result; });

    EXPECT_FALSE(saved);
    EXPECT_TRUE(controller.hasUnsavedChanges());
    EXPECT_FALSE(controller.currentFolder().has_value());
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Speichern fehlgeschlagen");
    EXPECT_THAT(view.messages[0].text, HasSubstr("Notizen 2026-10-17 09-40"));
}

TEST_F(DocumentControllerTest, FPJ02_SaveAsOfANewProjectSavesInTheChosenFolder)
{
    makeChange();
    EXPECT_CALL(repository, save(tree, rehearsal)).WillOnce(Return(ProjectFileError::none));
    EXPECT_CALL(repository, saveCopy(_, _, _)).Times(0);

    saveAs(rehearsal);

    EXPECT_EQ(controller.currentFolder(), rehearsal);
    EXPECT_EQ(project.name(), "Probe");
    EXPECT_EQ(view.windowTitle, "Probe – Scratch Notes");
    EXPECT_THAT(view.suggestedFolders, ElementsAre(projectsFolder / "Notizen 2026-10-17 09-40"));
}

TEST_F(DocumentControllerTest, FPJ02_SaveAsOfASavedProjectCopiesItsAudio)
{
    saveAs(rehearsal);
    const auto copy = fs::path{"Band"} / "Probe Kopie";
    EXPECT_CALL(repository, saveCopy(tree, rehearsal, copy)).WillOnce(Return(ProjectFileError::none));

    saveAs(copy);

    EXPECT_EQ(controller.currentFolder(), copy);
    EXPECT_EQ(project.name(), "Probe Kopie");
}

TEST_F(DocumentControllerTest, FPJ02_FailedSaveAsKeepsTheOldFolder)
{
    saveAs(rehearsal);
    EXPECT_CALL(repository, saveCopy(_, _, _)).WillOnce(Return(ProjectFileError::writeFailed));
    bool saved = true;
    view.folderToSave = fs::path{"voll"};

    controller.saveAs([&saved](bool result) { saved = result; });

    EXPECT_FALSE(saved);
    EXPECT_EQ(controller.currentFolder(), rehearsal);
    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_THAT(view.messages[0].text, HasSubstr("voll"));
}

TEST_F(DocumentControllerTest, FPJ02_CancelledSaveAsSavesNothing)
{
    makeChange();
    EXPECT_CALL(repository, save(_, _)).Times(0);
    bool saved = true;

    controller.saveAs([&saved](bool result) { saved = result; });

    EXPECT_FALSE(saved);
    EXPECT_TRUE(controller.hasUnsavedChanges());
}

TEST_F(DocumentControllerTest, FAU02_WithFolderSavesANewProjectFirst)
{
    EXPECT_CALL(repository, save(_, projectsFolder / "Notizen 2026-10-17 09-40"))
        .WillOnce(Return(ProjectFileError::none));
    std::optional<fs::path> reported;

    controller.withFolder([&reported](const std::optional<fs::path>& folder) { reported = folder; });

    EXPECT_EQ(reported, projectsFolder / "Notizen 2026-10-17 09-40");
}

TEST_F(DocumentControllerTest, FAU02_WithFolderReportsTheFolderOfASavedProject)
{
    saveAs(rehearsal);
    EXPECT_CALL(repository, save(_, _)).Times(0);
    std::optional<fs::path> reported;

    controller.withFolder([&reported](const std::optional<fs::path>& folder) { reported = folder; });

    EXPECT_EQ(reported, rehearsal);
}

TEST_F(DocumentControllerTest, FAU02_WithFolderReportsNothingIfSavingFailed)
{
    EXPECT_CALL(repository, save(_, _)).WillOnce(Return(ProjectFileError::writeFailed));
    std::optional<fs::path> reported = rehearsal;

    controller.withFolder([&reported](const std::optional<fs::path>& folder) { reported = folder; });

    EXPECT_FALSE(reported.has_value());
}

// ----- Open ---------------------------------------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ02_OpenLoadsTheFolderOfTheChosenFile)
{
    view.fileToOpen = rehearsal / "projekt.snotes";
    EXPECT_CALL(repository, load(rehearsal)).WillOnce(Return(loadableProject("Probe")));

    controller.open();

    EXPECT_EQ(project.name(), "Probe");
    EXPECT_EQ(controller.currentFolder(), rehearsal);
    EXPECT_THAT(view.openStartFolders, ElementsAre(projectsFolder));
    EXPECT_THAT(remembered, ElementsAre(rehearsal));
}

TEST_F(DocumentControllerTest, FPJ02_OpenStartsInTheCurrentFolder)
{
    saveAs(rehearsal);

    controller.open();

    EXPECT_THAT(view.openStartFolders, ElementsAre(rehearsal));
}

TEST_F(DocumentControllerTest, FPJ04_OpenedProjectStartsWithoutUndoHistory)
{
    makeChange();
    controller.save();
    view.fileToOpen = rehearsal / "projekt.snotes";
    EXPECT_CALL(repository, load(_)).WillOnce(Return(loadableProject("Probe")));

    controller.open();

    EXPECT_FALSE(controller.canUndo());
    EXPECT_FALSE(controller.hasUnsavedChanges());
}

TEST_F(DocumentControllerTest, FPJ02_CancelledOpenKeepsTheProject)
{
    EXPECT_CALL(repository, load(_)).Times(0);

    controller.open();

    EXPECT_EQ(project.name(), "Notizen 2026-10-17 09-40");
}

TEST_F(DocumentControllerTest, FPJ02_OpenReportsLoadErrors)
{
    view.fileToOpen = rehearsal / "projekt.snotes";
    EXPECT_CALL(repository, load(_))
        .WillOnce(Return(LoadResult{{}, ProjectFileError::unreadable, {}}))
        .WillOnce(Return(LoadResult{{}, ProjectFileError::notAProject, {}}))
        .WillOnce(Return(LoadResult{{}, ProjectFileError::newerFormatVersion, {}}));

    controller.open();
    controller.open();
    controller.open();

    ASSERT_EQ(view.messages.size(), 3U);
    EXPECT_EQ(view.messages[0].text, "„Probe“ enthält keine lesbare projekt.snotes.");
    EXPECT_EQ(view.messages[1].text, "Die projekt.snotes in „Probe“ ist kein Scratch-Notes-Projekt.");
    EXPECT_THAT(view.messages[2].text, HasSubstr("neueren Version"));
    EXPECT_FALSE(controller.currentFolder().has_value());
}

TEST_F(DocumentControllerTest, FPJ03_OpenReportsMissingAudio)
{
    view.fileToOpen = rehearsal / "projekt.snotes";
    auto result = loadableProject("Probe");
    result.missingAudio = {{"Bass", "audio/t1-di.wav"}, {"Gitarre", "audio/t2-wet.wav"}};
    EXPECT_CALL(repository, load(_)).WillOnce(Return(result));

    controller.open();

    ASSERT_EQ(view.messages.size(), 1U);
    EXPECT_EQ(view.messages[0].title, "Fehlende Audiodateien");
    EXPECT_THAT(view.messages[0].text, HasSubstr("Bass: audio/t1-di.wav\nGitarre: audio/t2-wet.wav\n"));
    EXPECT_EQ(controller.currentFolder(), rehearsal);
}

// ----- Unsaved changes ----------------------------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ01_NewProjectWithoutChangesAsksNothing)
{
    saveAs(rehearsal);

    controller.newProject();

    EXPECT_TRUE(view.askedToSave.empty());
    EXPECT_FALSE(controller.currentFolder().has_value());
    EXPECT_EQ(project.numVoices(), 0);
}

TEST_F(DocumentControllerTest, FPJ01_CancelKeepsTheChangedProject)
{
    makeChange();
    view.saveChangesAnswer = SaveChangesChoice::cancel;

    controller.newProject();

    EXPECT_THAT(view.askedToSave, ElementsAre("Notizen 2026-10-17 09-40"));
    EXPECT_EQ(project.numVoices(), 1);
}

TEST_F(DocumentControllerTest, FPJ01_DiscardDropsTheChanges)
{
    makeChange();
    view.saveChangesAnswer = SaveChangesChoice::discard;
    EXPECT_CALL(repository, save(_, _)).Times(0);

    controller.newProject();

    EXPECT_EQ(project.numVoices(), 0);
}

TEST_F(DocumentControllerTest, FPJ01_SaveSavesBeforeTheNewProject)
{
    makeChange();
    view.saveChangesAnswer = SaveChangesChoice::save;
    EXPECT_CALL(repository, save(_, _)).WillOnce(Return(ProjectFileError::none));

    controller.newProject();

    EXPECT_EQ(project.numVoices(), 0);
}

TEST_F(DocumentControllerTest, FPJ01_FailedSaveKeepsTheChangedProject)
{
    makeChange();
    view.saveChangesAnswer = SaveChangesChoice::save;
    EXPECT_CALL(repository, save(_, _)).WillOnce(Return(ProjectFileError::writeFailed));

    controller.newProject();

    EXPECT_EQ(project.numVoices(), 1);
}

TEST_F(DocumentControllerTest, FPJ02_CloseAsksForUnsavedChanges)
{
    bool closed = false;
    controller.requestClose([&closed] { closed = true; });
    EXPECT_TRUE(closed);

    makeChange();
    closed = false;
    controller.requestClose([&closed] { closed = true; });
    EXPECT_FALSE(closed);
    EXPECT_EQ(view.askedToSave.size(), 1U);
}

// ----- Undo ---------------------------------------------------------------------------------------

TEST_F(DocumentControllerTest, FPJ04_UndoAndRedo)
{
    EXPECT_FALSE(controller.canUndo());
    makeChange();
    EXPECT_TRUE(controller.canUndo());

    controller.undo();
    EXPECT_EQ(project.numVoices(), 0);
    EXPECT_TRUE(controller.canRedo());

    controller.redo();
    EXPECT_EQ(project.numVoices(), 1);
    EXPECT_FALSE(controller.canRedo());
}

} // namespace
} // namespace snotes::ui
