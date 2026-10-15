#include "ui/AboutInfo.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>

namespace snotes::ui
{
namespace
{

using ::testing::HasSubstr;
using ::testing::StartsWith;

const ThirdPartyComponent* find(const AboutInfo& info, const std::string& namePart)
{
    const auto found = std::find_if(info.components.begin(),
                                    info.components.end(),
                                    [&namePart](const auto& component)
                                    { return component.name.find(namePart) != std::string::npos; });
    return found == info.components.end() ? nullptr : &*found;
}

TEST(AboutInfoTest, Q12_ShowsVersionOwnLicenseSourceCodeAndClaude)
{
    const auto info = makeAboutInfo("0.1.0", false);

    EXPECT_EQ(info.title, "Scratch Notes 0.1.0");
    ASSERT_EQ(info.links.size(), 3U);
    EXPECT_EQ(info.links[0].url, "https://github.com/ladych/ScratchNotes");
    EXPECT_THAT(info.links[1].url, HasSubstr("agpl-3.0"));
    EXPECT_THAT(info.lines.back(), HasSubstr("Claude"));
}

TEST(AboutInfoTest, Q12_NamesJuceOnnxRuntimeAndVerovioWithTheirLicenses)
{
    const auto info = makeAboutInfo("0.1.0", false);

    EXPECT_NE(find(info, "JUCE"), nullptr);
    EXPECT_EQ(find(info, "ONNX Runtime")->licenseText.text, "MIT");
    EXPECT_THAT(find(info, "ONNX Runtime")->notice, HasSubstr("Microsoft"));
    EXPECT_EQ(find(info, "Verovio")->licenseText.text, "LGPLv3");
    EXPECT_NE(find(info, "IJG")->notice, "");
}

TEST(AboutInfoTest, Q12_TheAsioSdkAndItsTrademarkNoticeOnlyInTheAsioBuild)
{
    EXPECT_EQ(find(makeAboutInfo("1.0", false), "ASIO"), nullptr);

    const auto info = makeAboutInfo("1.0", true);
    const auto* asio = find(info, "ASIO");
    ASSERT_NE(asio, nullptr);
    EXPECT_THAT(asio->notice, HasSubstr("trademark"));
    EXPECT_EQ(asio->licenseText.text, "GPLv3");
}

TEST(AboutInfoTest, Q12_EveryComponentHasAuthorLicenseAndWebLinks)
{
    for (const auto& component : makeAboutInfo("1.0", true).components)
    {
        EXPECT_FALSE(component.author.empty()) << component.name;
        EXPECT_FALSE(component.license.empty()) << component.name;
        EXPECT_THAT(component.website.url, StartsWith("http")) << component.name;
        EXPECT_THAT(component.licenseText.url, StartsWith("http")) << component.name;
    }
}

} // namespace
} // namespace snotes::ui
