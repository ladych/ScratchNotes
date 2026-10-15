#include "i18n/Translate.h"

#include "i18n/TestLanguage.h"

#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace snotes::i18n
{
namespace
{

TEST(Translate, FUI01_ShowsTheTextInTheCurrentLanguage)
{
    EXPECT_EQ(tr("Test tone"), "Testton");
    EXPECT_EQ(trJuce("Test tone"), juce::String::fromUTF8("Testton"));

    const test::ScopedEnglish english;
    EXPECT_EQ(tr("Test tone"), "Test tone");
    EXPECT_EQ(trJuce("Test tone"), "Test tone");
}

TEST(Translate, FUI03_KeepsTextsWithoutTranslationEnglish)
{
    EXPECT_EQ(tr("No such text"), "No such text");
}

TEST(Translate, FUI01_ReplacesPlaceholdersAfterTranslating)
{
    EXPECT_EQ(tr("Replacing %1 | ", "UR44C"), "Ersatz für UR44C | ");
    EXPECT_EQ(trJuce("%1 inputs", "4"), juce::String::fromUTF8("4 Eingänge"));
}

TEST(Translate, FUI01_DecimalSeparatorFollowsTheLanguage)
{
    EXPECT_EQ(decimalSeparator(), ",");
    const test::ScopedEnglish english;
    EXPECT_EQ(decimalSeparator(), ".");
}

TEST(Translate, FUI01_SubstitutesEachPlaceholderOnce)
{
    EXPECT_EQ(substitute("%2 before %1", std::array<std::string_view, 2>{"a", "b"}), "b before a");
    // An argument's own "%2" is not replaced again.
    EXPECT_EQ(substitute("%1 and %2", std::array<std::string_view, 2>{"%2", "x"}), "%2 and x");
    EXPECT_EQ(substitute("%1 %1", std::array<std::string_view, 1>{"a"}), "a a");
}

TEST(Translate, FUI01_LeavesOtherPercentSignsAsTheyAre)
{
    EXPECT_EQ(substitute("100% %0 %a %3 %", std::array<std::string_view, 2>{"a", "b"}), "100% %0 %a %3 %");
    EXPECT_EQ(substitute("no placeholders", {}), "no placeholders");
}

TEST(Translate, FUI01_MarkedTextIsTheEnglishText)
{
    static_assert(marked("Recording")[0] == 'R');
    EXPECT_STREQ(marked("Recording"), "Recording");
}

} // namespace
} // namespace snotes::i18n
