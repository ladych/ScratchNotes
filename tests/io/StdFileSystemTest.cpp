#include "io/StdFileSystem.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <span>
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
    const auto temp = root / "projekt.snotes.tmp";
    const auto target = root / "projekt.snotes";
    ASSERT_TRUE(fileSystem.writeText(target, "old"));
    ASSERT_TRUE(fileSystem.writeText(temp, "new"));

    EXPECT_TRUE(fileSystem.rename(temp, target));

    EXPECT_EQ(fileSystem.readText(target), std::optional<std::string>{"new"});
    EXPECT_FALSE(fs::exists(temp));
}

TEST_F(StdFileSystemTest, Q10_RenamingMissingFileFails)
{
    EXPECT_FALSE(fileSystem.rename(root / "missing.tmp", root / "projekt.snotes"));
}

TEST_F(StdFileSystemTest, FPJ02_CopyFileReplacesTheTarget)
{
    const auto source = root / "take-di.wav";
    const auto target = root / "copy.wav";
    ASSERT_TRUE(fileSystem.writeText(source, "audio"));
    ASSERT_TRUE(fileSystem.writeText(target, "old"));

    EXPECT_TRUE(fileSystem.copyFile(source, target));

    EXPECT_EQ(fileSystem.readText(target), std::optional<std::string>{"audio"});
    EXPECT_TRUE(fs::exists(source));
}

TEST_F(StdFileSystemTest, FPJ02_CopyingMissingFileFails)
{
    EXPECT_FALSE(fileSystem.copyFile(root / "missing.wav", root / "copy.wav"));
}

TEST_F(StdFileSystemTest, FAU02_BinaryFileAppendsAndOverwrites)
{
    const auto file = root / "take.wav";
    auto binary = fileSystem.createBinaryFile(file);
    ASSERT_NE(binary, nullptr);
    const std::string head = "abcd";
    const std::string tail = "ef";
    const std::string patch = "XY";

    EXPECT_TRUE(binary->append(std::as_bytes(std::span{head})));
    EXPECT_TRUE(binary->append(std::as_bytes(std::span{tail})));
    EXPECT_TRUE(binary->overwrite(1, std::as_bytes(std::span{patch})));
    EXPECT_TRUE(binary->flush());

    EXPECT_EQ(fileSystem.readText(file), std::optional<std::string>{"aXYdef"});
}

TEST_F(StdFileSystemTest, FAU02_BinaryFileInMissingFolderFails)
{
    EXPECT_EQ(fileSystem.createBinaryFile(root / "missing" / "take.wav"), nullptr);
}

} // namespace
} // namespace snotes::io
