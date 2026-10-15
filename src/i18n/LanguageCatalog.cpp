#include "i18n/LanguageCatalog.h"

#include "i18n/Translate.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <cstddef>
#include <memory>

namespace snotes::i18n
{
namespace
{

/// "de_DE", "de-AT" or "DE" -> "de".
std::string languagePart(const std::string& locale)
{
    return juce::String{locale}
        .upToFirstOccurrenceOf("_", false, false)
        .upToFirstOccurrenceOf("-", false, false)
        .trim()
        .toLowerCase()
        .toStdString();
}

} // namespace

void LanguageCatalog::add(const std::string& code, const std::string& fileText)
{
    const juce::LocalisedStrings strings{juce::String::fromUTF8(fileText.c_str()), false};
    auto name = strings.getLanguageName().toStdString();
    languages_.push_back({code, name.empty() ? code : name});
    fileTexts_.push_back(fileText);
}

bool LanguageCatalog::contains(const std::string& code) const
{
    return std::ranges::any_of(languages_,
                               [&code](const Language& language) { return language.code == code; });
}

std::string LanguageCatalog::resolve(const std::string& setting, const std::string& systemLanguage) const
{
    auto chosen = juce::String{setting}.trim().toStdString();
    if (contains(chosen))
        return chosen;
    auto system = languagePart(systemLanguage);
    if (contains(system))
        return system;
    return kEnglish;
}

std::vector<LanguageChoice> LanguageCatalog::choices(const std::string& setting,
                                                     const std::string& systemLanguage) const
{
    const auto chosen = juce::String{setting}.trim().toStdString();
    const bool followsSystem = !contains(chosen);
    std::vector<LanguageChoice> result;
    result.push_back({tr("System language (%1)", nameOf(resolve(kSystemLanguage, systemLanguage))),
                      kSystemLanguage,
                      followsSystem});
    for (const auto& language : languages_)
        result.push_back({language.name, language.code, !followsSystem && language.code == chosen});
    return result;
}

std::string LanguageCatalog::restartNotice(const std::string& newSetting,
                                           const std::string& systemLanguage) const
{
    return translate(resolve(newSetting, systemLanguage),
                     marked("The language changes when Scratch Notes is started the next time."));
}

// JUCE's setCurrentMappings() takes ownership, which the static analyzer does not see.
// NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
void LanguageCatalog::apply(const std::string& code) const
{
    const auto* text = fileTextOf(code);
    if (text == nullptr)
    {
        juce::LocalisedStrings::setCurrentMappings(nullptr);
        return;
    }
    auto mappings = std::make_unique<juce::LocalisedStrings>(juce::String{*text}, false);
    juce::LocalisedStrings::setCurrentMappings(mappings.release());
}
// NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

std::string LanguageCatalog::translate(const std::string& code, const char* english) const
{
    const auto original = juce::String::fromUTF8(english);
    const auto* text = fileTextOf(code);
    if (text == nullptr)
        return original.toStdString();
    const juce::LocalisedStrings strings{juce::String::fromUTF8(text->c_str()), false};
    return strings.translate(original).toStdString();
}

const std::string& LanguageCatalog::nameOf(const std::string& code) const
{
    for (std::size_t index = 1; index < languages_.size(); ++index)
        if (languages_[index].code == code)
            return languages_[index].name;
    return languages_.front().name; // English
}

const std::string* LanguageCatalog::fileTextOf(const std::string& code) const
{
    for (std::size_t index = 1; index < languages_.size(); ++index)
        if (languages_[index].code == code)
            return &fileTexts_[index - 1];
    return nullptr;
}

} // namespace snotes::i18n
