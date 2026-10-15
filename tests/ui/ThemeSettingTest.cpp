#include "ui/ThemeSetting.h"

#include "i18n/TestLanguage.h"
#include "io/MockFileSystem.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{
namespace
{

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

const std::filesystem::path kThemeFile{"config/theme.txt"};

class ThemeSettingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, writeText(kThemeFile, _))
            .WillByDefault(
                [this](const std::filesystem::path& /*file*/, const std::string& text)
                {
                    saved = text;
                    return true;
                });
        ON_CALL(fileSystem, readText(kThemeFile)).WillByDefault([this] { return saved; });
    }

    ThemeSetting make()
    {
        return ThemeSetting{
            store, [this] { return systemIsDark; }, [this](Theme theme) { shown.push_back(theme); }};
    }

    NiceMock<io::MockFileSystem> fileSystem;
    io::SettingsStore store{fileSystem, kThemeFile};
    std::optional<std::string> saved;
    bool systemIsDark = true;
    std::vector<Theme> shown;
};

TEST_F(ThemeSettingTest, FUI04_WithoutSettingTheThemeFollowsTheSystem)
{
    systemIsDark = false;
    const auto setting = make();

    EXPECT_EQ(setting.theme(), Theme::light);
    systemIsDark = true;
    EXPECT_EQ(setting.theme(), Theme::dark);
}

TEST_F(ThemeSettingTest, FUI04_SavedSettingWinsOverTheSystem)
{
    saved = "light\n";
    const auto setting = make();

    EXPECT_EQ(setting.theme(), Theme::light);
    saved = "dark";
    systemIsDark = false;
    EXPECT_EQ(make().theme(), Theme::dark);
}

TEST_F(ThemeSettingTest, FUI04_UnknownSettingFollowsTheSystem)
{
    saved = "purple";
    systemIsDark = false;

    EXPECT_EQ(make().theme(), Theme::light);
}

TEST_F(ThemeSettingTest, FUI04_ApplyShowsTheResolvedTheme)
{
    saved = "light";
    const auto setting = make();

    setting.apply();

    EXPECT_EQ(shown, std::vector<Theme>{Theme::light});
}

TEST_F(ThemeSettingTest, FUI05_ChoicesNameTheSystemThemeAndTickTheSetting)
{
    i18n::test::ScopedEnglish english;
    const auto choices = make().choices();

    ASSERT_EQ(choices.size(), 3U);
    EXPECT_EQ(choices[0].label, "System setting (dark)");
    EXPECT_EQ(choices[0].setting, ThemeSetting::kSystem);
    EXPECT_TRUE(choices[0].ticked);
    EXPECT_EQ(choices[1].label, "Dark");
    EXPECT_EQ(choices[1].setting, ThemeSetting::kDark);
    EXPECT_FALSE(choices[1].ticked);
    EXPECT_EQ(choices[2].label, "Light");
    EXPECT_EQ(choices[2].setting, ThemeSetting::kLight);
    EXPECT_FALSE(choices[2].ticked);
}

TEST_F(ThemeSettingTest, FUI05_ChoicesAreTranslated)
{
    saved = "light";
    systemIsDark = false;
    const auto choices = make().choices();

    EXPECT_EQ(choices[0].label, "Systemeinstellung (hell)");
    EXPECT_FALSE(choices[0].ticked);
    EXPECT_EQ(choices[1].label, "Dunkel");
    EXPECT_EQ(choices[2].label, "Hell");
    EXPECT_TRUE(choices[2].ticked);
}

TEST_F(ThemeSettingTest, FUI05_ChoosingSavesAndShowsTheThemeAtOnce)
{
    auto setting = make();

    setting.choose(ThemeSetting::kLight);

    EXPECT_EQ(saved, std::optional<std::string>{"light"});
    EXPECT_EQ(shown, std::vector<Theme>{Theme::light});
    EXPECT_EQ(setting.theme(), Theme::light);
}

TEST_F(ThemeSettingTest, FUI05_ChoosingTheSystemSettingFollowsTheSystemAgain)
{
    saved = "light";
    auto setting = make();

    setting.choose(ThemeSetting::kSystem);

    EXPECT_EQ(saved, std::optional<std::string>{""});
    EXPECT_EQ(shown, std::vector<Theme>{Theme::dark});
}

TEST_F(ThemeSettingTest, FUI04_SystemChangeIsShownWhenTheSettingFollowsTheSystem)
{
    const auto setting = make();

    systemIsDark = false;
    setting.systemChanged();

    EXPECT_EQ(shown, std::vector<Theme>{Theme::light});
}

TEST_F(ThemeSettingTest, FUI04_SystemChangeIsIgnoredWithAFixedTheme)
{
    saved = "dark";
    const auto setting = make();

    systemIsDark = false;
    setting.systemChanged();

    EXPECT_TRUE(shown.empty());
}

} // namespace
} // namespace snotes::ui
