#include "io/StdFileSystem.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace snotes::io
{
namespace
{

namespace fs = std::filesystem;

class StdFileSystemTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        root = fs::temp_directory_path() / "snotes_tests" / testInfo->name();
        fs::remove_all(root);
        fs::create_directories(root);
    }

    void TearDown() override { fs::remove_all(root); }

    fs::path root;
    StdFileSystem fileSystem;
};

TEST_F(StdFileSystemTest, FAO04_ReadsBackWrittenText)
{
    const auto file = root / "settings.xml";

    ASSERT_TRUE(fileSystem.writeText(file, "line 1\nline 2\n"));

    EXPECT_EQ(fileSystem.readText(file), std::optional<std::string>{"line 1\nline 2\n"});
}

TEST_F(StdFileSystemTest, FAO04_OverwritesExistingFile)
{
    const auto file = root / "settings.xml";
    ASSERT_TRUE(fileSystem.writeText(file, "old content that is longer"));

    ASSERT_TRUE(fileSystem.writeText(file, "new"));

    EXPECT_EQ(fileSystem.readText(file), std::optional<std::string>{"new"});
}

TEST_F(StdFileSystemTest, FAO04_ReadingMissingFileReturnsNothing)
{
    EXPECT_EQ(fileSystem.readText(root / "missing.xml"), std::nullopt);
}

TEST_F(StdFileSystemTest, FAO04_WritingIntoMissingDirectoryFails)
{
    EXPECT_FALSE(fileSystem.writeText(root / "missing" / "settings.xml", "x"));
}

TEST_F(StdFileSystemTest, FAO04_CreatesNestedDirectories)
{
    const auto directory = root / "a" / "b";

    EXPECT_TRUE(fileSystem.createDirectories(directory));
    EXPECT_TRUE(fs::is_directory(directory));
}

TEST_F(StdFileSystemTest, FAO04_CreatingDirectoriesFailsWhenPathIsAFile)
{
    const auto blocker = root / "blocker";
    std::ofstream{blocker} << "x";

    EXPECT_FALSE(fileSystem.createDirectories(blocker / "child"));
}

TEST_F(StdFileSystemTest, FPJ03_TellsWhetherAFileExists)
{
    const auto file = root / "kick.wav";
    std::ofstream{file} << "x";

    EXPECT_TRUE(fileSystem.exists(file));
    EXPECT_FALSE(fileSystem.exists(root / "snare.wav"));
}

TEST_F(StdFileSystemTest, Q10_RenameReplacesTheTargetFile)
{
    const auto temp = root / "rock.dpp.tmp";
    const auto target = root / "rock.dpp";
    ASSERT_TRUE(fileSystem.writeText(target, "old"));
    ASSERT_TRUE(fileSystem.writeText(temp, "new"));

    EXPECT_TRUE(fileSystem.rename(temp, target));

    EXPECT_EQ(fileSystem.readText(target), std::optional<std::string>{"new"});
    EXPECT_FALSE(fs::exists(temp));
}

TEST_F(StdFileSystemTest, Q10_RenamingMissingFileFails)
{
    EXPECT_FALSE(fileSystem.rename(root / "missing.tmp", root / "rock.dpp"));
}

} // namespace
} // namespace snotes::io
