#include "io/ProjectRepository.h"

#include "io/MockFileSystem.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

namespace snotes::io
{
namespace
{

namespace fs = std::filesystem;
namespace ids = model::ids;
using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

class ProjectRepositoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto voice = project.addVoice({.id = "v1", .name = "Bass", .profileId = "bass4-e", .inputDi = 0});
        voice.addTake({.id = "t1", .diFile = "audio/t1-di.wav", .wetFile = "audio/t1-wet.wav"});
        voice.addTake({.id = "t2", .diFile = "audio/t2-di.wav"});
        ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, writeText(_, _)).WillByDefault(Return(true));
        ON_CALL(fileSystem, rename(_, _)).WillByDefault(Return(true));
        ON_CALL(fileSystem, copyFile(_, _)).WillByDefault(Return(true));
        ON_CALL(fileSystem, exists(_)).WillByDefault(Return(true));
    }

    fs::path folder = fs::path{"projects"} / "Probe";
    fs::path copyFolder = fs::path{"projects"} / "Kopie";
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, nullptr};
    NiceMock<MockFileSystem> fileSystem;
    ProjectRepository repository{fileSystem};
};

TEST_F(ProjectRepositoryTest, Q10_SaveWritesATemporaryFileAndRenamesIt)
{
    const auto file = folder / "projekt.snotes";
    auto temp = file;
    temp += ".tmp";
    {
        InSequence order;
        EXPECT_CALL(fileSystem, createDirectories(folder / "audio")).WillOnce(Return(true));
        EXPECT_CALL(fileSystem, writeText(temp, ProjectSerializer::toXml(tree))).WillOnce(Return(true));
        EXPECT_CALL(fileSystem, rename(temp, file)).WillOnce(Return(true));
    }

    EXPECT_EQ(repository.save(tree, folder), ProjectFileError::none);
}

TEST_F(ProjectRepositoryTest, Q10_FailedWriteLeavesTheOldFile)
{
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, rename(_, _)).Times(0);

    EXPECT_EQ(repository.save(tree, folder), ProjectFileError::writeFailed);
}

TEST_F(ProjectRepositoryTest, Q10_FailedRenameIsReported)
{
    EXPECT_CALL(fileSystem, rename(_, _)).WillOnce(Return(false));

    EXPECT_EQ(repository.save(tree, folder), ProjectFileError::writeFailed);
}

TEST_F(ProjectRepositoryTest, FPJ02_SaveFailsIfTheFolderCannotBeCreated)
{
    EXPECT_CALL(fileSystem, createDirectories(_)).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, writeText(_, _)).Times(0);

    EXPECT_EQ(repository.save(tree, folder), ProjectFileError::writeFailed);
}

TEST_F(ProjectRepositoryTest, FPJ02_LoadReadsTheProjectFileOfTheFolder)
{
    EXPECT_CALL(fileSystem, readText(folder / "projekt.snotes"))
        .WillOnce(Return(ProjectSerializer::toXml(tree)));

    const auto result = repository.load(folder);

    ASSERT_EQ(result.error, ProjectFileError::none);
    EXPECT_TRUE(result.project.isEquivalentTo(tree));
    EXPECT_TRUE(result.missingAudio.empty());
}

TEST_F(ProjectRepositoryTest, FPJ02_UnreadableFolderIsReported)
{
    EXPECT_CALL(fileSystem, readText(_)).WillOnce(Return(std::nullopt));

    EXPECT_EQ(repository.load(folder).error, ProjectFileError::unreadable);
}

TEST_F(ProjectRepositoryTest, FPJ02_InvalidFileIsReported)
{
    EXPECT_CALL(fileSystem, readText(_)).WillOnce(Return(std::string{"<PROJECT/>"}));

    EXPECT_EQ(repository.load(folder).error, ProjectFileError::notAProject);
}

TEST_F(ProjectRepositoryTest, FPJ03_MissingAudioIsMarkedAndReported)
{
    EXPECT_CALL(fileSystem, readText(_)).WillOnce(Return(ProjectSerializer::toXml(tree)));
    EXPECT_CALL(fileSystem, exists(_)).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, exists(folder / "audio" / "t1-wet.wav")).WillOnce(Return(false));

    const auto result = repository.load(folder);

    ASSERT_EQ(result.missingAudio.size(), 1U);
    EXPECT_EQ(result.missingAudio[0].voiceName, "Bass");
    EXPECT_EQ(result.missingAudio[0].file, "audio/t1-wet.wav");
    const model::Project loaded{result.project, nullptr};
    EXPECT_TRUE(loaded.voice(0).take(0).isAudioMissing());
    EXPECT_FALSE(loaded.voice(0).take(1).isAudioMissing());
}

TEST_F(ProjectRepositoryTest, FPJ02_SaveCopyCopiesTheTakeAudio)
{
    EXPECT_CALL(fileSystem, createDirectories(copyFolder / "audio")).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, copyFile(folder / "audio" / "t1-di.wav", copyFolder / "audio" / "t1-di.wav"));
    EXPECT_CALL(fileSystem, copyFile(folder / "audio" / "t1-wet.wav", copyFolder / "audio" / "t1-wet.wav"));
    EXPECT_CALL(fileSystem, copyFile(folder / "audio" / "t2-di.wav", copyFolder / "audio" / "t2-di.wav"));
    EXPECT_CALL(fileSystem, rename(_, copyFolder / "projekt.snotes")).WillOnce(Return(true));

    EXPECT_EQ(repository.saveCopy(tree, folder, copyFolder), ProjectFileError::none);
}

TEST_F(ProjectRepositoryTest, FPJ03_SaveCopySkipsMissingAudio)
{
    auto take = project.voice(0).take(1).tree();
    take.setProperty(ids::audioMissing, true, nullptr);
    EXPECT_CALL(fileSystem, copyFile(_, _)).Times(2).WillRepeatedly(Return(true));

    EXPECT_EQ(repository.saveCopy(tree, folder, copyFolder), ProjectFileError::none);
}

TEST_F(ProjectRepositoryTest, FPJ02_SaveCopyFailsIfAudioCannotBeCopied)
{
    EXPECT_CALL(fileSystem, copyFile(_, _)).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, writeText(_, _)).Times(0);

    EXPECT_EQ(repository.saveCopy(tree, folder, copyFolder), ProjectFileError::writeFailed);
}

TEST_F(ProjectRepositoryTest, FPJ02_SaveCopyFailsIfTheFolderCannotBeCreated)
{
    EXPECT_CALL(fileSystem, createDirectories(_)).WillOnce(Return(false));
    EXPECT_CALL(fileSystem, copyFile(_, _)).Times(0);

    EXPECT_EQ(repository.saveCopy(tree, folder, copyFolder), ProjectFileError::writeFailed);
}

TEST_F(ProjectRepositoryTest, FPJ01_FreeFolderKeepsAnUnusedName)
{
    EXPECT_CALL(fileSystem, exists(folder)).WillOnce(Return(false));

    EXPECT_EQ(repository.freeFolder(folder), folder);
}

TEST_F(ProjectRepositoryTest, FPJ01_FreeFolderNumbersUsedNames)
{
    EXPECT_CALL(fileSystem, exists(folder)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, exists(fs::path{"projects"} / "Probe (2)")).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, exists(fs::path{"projects"} / "Probe (3)")).WillOnce(Return(false));

    EXPECT_EQ(repository.freeFolder(folder), fs::path{"projects"} / "Probe (3)");
}

} // namespace
} // namespace snotes::io
