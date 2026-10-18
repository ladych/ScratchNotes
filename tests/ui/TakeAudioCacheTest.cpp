#include "ui/TakeAudioCache.h"

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace snotes::ui
{
namespace
{

namespace fs = std::filesystem;

class FakeLoader final : public ITakeAudioLoader
{
public:
    [[nodiscard]] std::shared_ptr<const engine::TakeAudio> load(const fs::path& file) override
    {
        loaded.push_back(file.generic_string());
        if (file.filename() == "weg.wav")
            return nullptr;
        return std::make_shared<const engine::TakeAudio>(std::vector<float>{0.5F});
    }

    std::vector<std::string> loaded;
};

class TakeAudioCacheTest : public ::testing::Test
{
protected:
    FakeLoader loader;
    std::optional<fs::path> folder = fs::path{"Band"};
    TakeAudioCache cache{loader, [this] { return folder; }};
};

TEST_F(TakeAudioCacheTest, FWI02_LoadsTheFileFromTheProjectFolder)
{
    const auto audio = cache.audio("audio/a.wav");

    ASSERT_NE(audio, nullptr);
    EXPECT_EQ(loader.loaded, std::vector<std::string>{"Band/audio/a.wav"});
}

TEST_F(TakeAudioCacheTest, FWI02_LoadsAFileOnceWhilePlayed)
{
    const auto first = cache.audio("audio/a.wav");
    const auto second = cache.audio("audio/a.wav");

    EXPECT_EQ(first, second);
    EXPECT_EQ(loader.loaded.size(), 1U);
    EXPECT_EQ(cache.size(), 1U);
}

TEST_F(TakeAudioCacheTest, Q04_FreesAudioNoSnapshotHolds)
{
    auto audio = cache.audio("audio/a.wav");
    audio.reset();

    EXPECT_EQ(cache.size(), 0U);
    static_cast<void>(cache.audio("audio/a.wav"));
    EXPECT_EQ(loader.loaded.size(), 2U);
}

TEST_F(TakeAudioCacheTest, FPJ03_UnreadableFileIsRetried)
{
    EXPECT_EQ(cache.audio("audio/weg.wav"), nullptr);
    EXPECT_EQ(cache.audio("audio/weg.wav"), nullptr);

    EXPECT_EQ(loader.loaded.size(), 2U);
}

TEST_F(TakeAudioCacheTest, FWI02_NothingWithoutFolderOrFile)
{
    EXPECT_EQ(cache.audio(""), nullptr);
    folder.reset();
    EXPECT_EQ(cache.audio("audio/a.wav"), nullptr);

    EXPECT_TRUE(loader.loaded.empty());
}

} // namespace
} // namespace snotes::ui
