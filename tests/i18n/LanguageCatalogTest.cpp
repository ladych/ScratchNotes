#include "i18n/LanguageCatalog.h"

#include "i18n/TestLanguage.h"
#include "i18n/Translate.h"

#include <gtest/gtest.h>

namespace snotes::i18n
{
namespace
{

constexpr const char* kFrench = "language: Français\ncountries: fr\n\"Test tone\" = \"Son test\"\n";

LanguageCatalog germanAndFrench()
{
    LanguageCatalog catalog;
    catalog.add("de", test::germanFileText());
    catalog.add("fr", kFrench);
    return catalog;
}

TEST(LanguageCatalog, FUI03_ListsEnglishFirstThenTheFilesInTheirOwnNames)
{
    const auto catalog = germanAndFrench();

    ASSERT_EQ(catalog.languages().size(), 3U);
    EXPECT_EQ(catalog.languages()[0].code, "en");
    EXPECT_EQ(catalog.languages()[0].name, "English");
    EXPECT_EQ(catalog.languages()[1].code, "de");
    EXPECT_EQ(catalog.languages()[1].name, "Deutsch");
    EXPECT_EQ(catalog.languages()[2].name, "Français");
    EXPECT_TRUE(catalog.contains("fr"));
    EXPECT_FALSE(catalog.contains("it"));
}

TEST(LanguageCatalog, FUI03_FileWithoutLanguageLineIsNamedByItsCode)
{
    LanguageCatalog catalog;
    catalog.add("nl", "\"Cut\" = \"Knippen\"\n");

    EXPECT_EQ(catalog.languages().at(1).name, "nl");
}

TEST(LanguageCatalog, FUI02_SettingWinsOverTheSystemLanguage)
{
    const auto catalog = germanAndFrench();

    EXPECT_EQ(catalog.resolve("fr", "de_DE"), "fr");
    EXPECT_EQ(catalog.resolve(" en\n", "de_DE"), "en");
}

TEST(LanguageCatalog, FUI02_WithoutSettingTheSystemLanguageIsShown)
{
    const auto catalog = germanAndFrench();

    EXPECT_EQ(catalog.resolve(LanguageCatalog::kSystemLanguage, "de"), "de");
    EXPECT_EQ(catalog.resolve(LanguageCatalog::kSystemLanguage, "de_DE"), "de");
    EXPECT_EQ(catalog.resolve(LanguageCatalog::kSystemLanguage, "FR-ca"), "fr");
}

TEST(LanguageCatalog, FUI02_UnknownLanguagesFallBackToEnglish)
{
    const auto catalog = germanAndFrench();

    EXPECT_EQ(catalog.resolve("it", "ja_JP"), "en");
    EXPECT_EQ(catalog.resolve({}, {}), "en");
}

TEST(LanguageCatalog, FUI01_ApplyChangesTheLanguageOfTr)
{
    const auto catalog = germanAndFrench();

    catalog.apply("fr");
    EXPECT_EQ(tr("Test tone"), "Son test");
    EXPECT_EQ(tr("Cut"), "Cut");
    catalog.apply("it");
    EXPECT_EQ(tr("Test tone"), "Test tone");
    catalog.apply("de");
    EXPECT_EQ(tr("Test tone"), "Testton");
    test::useGerman();
}

TEST(LanguageCatalog, FUI02_TranslatesIntoALanguageWithoutApplyingIt)
{
    const auto catalog = germanAndFrench();

    EXPECT_EQ(catalog.translate("fr", "Test tone"), "Son test");
    EXPECT_EQ(catalog.translate("fr", "Cut"), "Cut");
    EXPECT_EQ(catalog.translate("en", "Test tone"), "Test tone");
    EXPECT_EQ(tr("Test tone"), "Testton");
}

TEST(LanguageCatalog, FUI02_ChoicesOfferTheSystemLanguageThenEveryLanguage)
{
    const auto catalog = germanAndFrench();

    const auto choices = catalog.choices(LanguageCatalog::kSystemLanguage, "fr_FR");

    ASSERT_EQ(choices.size(), 4U);
    EXPECT_EQ(choices[0].label, "Systemsprache (Français)");
    EXPECT_EQ(choices[0].setting, LanguageCatalog::kSystemLanguage);
    EXPECT_TRUE(choices[0].ticked);
    EXPECT_EQ(choices[1].label, "English");
    EXPECT_EQ(choices[1].setting, "en");
    EXPECT_FALSE(choices[1].ticked);
    EXPECT_EQ(choices[2].label, "Deutsch");
    EXPECT_EQ(choices[3].label, "Français");
}

TEST(LanguageCatalog, FUI02_ChoicesTickTheSavedLanguage)
{
    const auto catalog = germanAndFrench();

    const auto choices = catalog.choices("de\n", "fr");

    EXPECT_FALSE(choices[0].ticked);
    EXPECT_FALSE(choices[1].ticked);
    EXPECT_TRUE(choices[2].ticked);
    EXPECT_FALSE(choices[3].ticked);
}

TEST(LanguageCatalog, FUI02_UnknownSettingTicksTheSystemLanguage)
{
    const auto catalog = germanAndFrench();

    const auto choices = catalog.choices("it", "ja");

    EXPECT_EQ(choices[0].label, "Systemsprache (English)");
    EXPECT_TRUE(choices[0].ticked);
}

TEST(LanguageCatalog, FUI02_RestartNoticeIsInTheNewLanguage)
{
    const auto catalog = germanAndFrench();

    EXPECT_EQ(catalog.restartNotice("en", "de"),
              "The language changes when Scratch Notes is started the next time.");
    EXPECT_EQ(catalog.restartNotice(LanguageCatalog::kSystemLanguage, "de_AT"),
              "Die Sprache wechselt beim nächsten Start von Scratch Notes.");
}

} // namespace
} // namespace snotes::i18n
