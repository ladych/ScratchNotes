#include "i18n/LanguageCatalog.h"
#include "i18n/TestLanguage.h"

#include <gtest/gtest.h>

#include <fstream>
#include <iterator>

namespace snotes::i18n::test
{
namespace
{

LanguageCatalog& catalog()
{
    static LanguageCatalog languages = []
    {
        LanguageCatalog catalog;
        catalog.add("de", germanFileText());
        return catalog;
    }();
    return languages;
}

class GermanTestLanguage final : public ::testing::Environment
{
public:
    void SetUp() override { useGerman(); }
    void TearDown() override { useEnglish(); }
};

const auto* const kEnvironment = ::testing::AddGlobalTestEnvironment(new GermanTestLanguage);

} // namespace

std::string germanFileText()
{
    std::ifstream file{SNOTES_LANGUAGES_DIR "/de.txt", std::ios::binary};
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

void useGerman()
{
    catalog().apply("de");
}

void useEnglish()
{
    catalog().apply(LanguageCatalog::kEnglish);
}

} // namespace snotes::i18n::test
