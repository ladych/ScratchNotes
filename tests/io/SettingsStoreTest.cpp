#include "io/SettingsStore.h"
#include "io/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace snotes::io
{
namespace
{

using ::testing::Return;
using ::testing::StrictMock;

const std::filesystem::path kSettingsDir{"config/ScratchNotes"};
const std::filesystem::path kSettingsFile = kSettingsDir / "audio-device.xml";

class SettingsStoreTest : public ::testing::Test
{
protected:
    StrictMock<MockFileSystem> fileSystem;
    SettingsStore store{fileSystem, kSettingsFile};
};

TEST_F(SettingsStoreTest, FAO04_RestoresPreviouslySavedState)
{
    EXPECT_CALL(fileSystem, readText(kSettingsFile)).WillOnce(Return(std::string{"<DEVICESETUP/>"}));

    EXPECT_EQ(store.load(), std::optional<std::string>{"<DEVICESETUP/>"});
}

TEST_F(SettingsStoreTest, FAO04_ReturnsNothingWhenNoStateWasSaved)
{
    EXPECT_CALL(fileSystem, readText(kSettingsFile)).WillOnce(Return(std::nullopt));

    EXPECT_EQ(store.load(), std::nullopt);
}

TEST_F(SettingsStoreTest, FAO04_TreatsEmptyFileAsNoSavedState)
{
    EXPECT_CALL(fileSystem, readText(kSettingsFile)).WillOnce(Return(std::string{}));

    EXPECT_EQ(store.load(), std::nullopt);
}

TEST_F(SettingsStoreTest, FAO04_SavesStateIntoSettingsDirectory)
{
    EXPECT_CALL(fileSystem, createDirectories(kSettingsDir)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, writeText(kSettingsFile, "<DEVICESETUP/>")).WillOnce(Return(true));

    EXPECT_TRUE(store.save("<DEVICESETUP/>"));
}

TEST_F(SettingsStoreTest, FAO04_ReportsFailureWhenDirectoryCannotBeCreated)
{
    EXPECT_CALL(fileSystem, createDirectories(kSettingsDir)).WillOnce(Return(false));

    EXPECT_FALSE(store.save("<DEVICESETUP/>"));
}

TEST_F(SettingsStoreTest, FAO04_ReportsFailureWhenWritingFails)
{
    EXPECT_CALL(fileSystem, createDirectories(kSettingsDir)).WillOnce(Return(true));
    EXPECT_CALL(fileSystem, writeText(kSettingsFile, "<DEVICESETUP/>")).WillOnce(Return(false));

    EXPECT_FALSE(store.save("<DEVICESETUP/>"));
}

} // namespace
} // namespace snotes::io
