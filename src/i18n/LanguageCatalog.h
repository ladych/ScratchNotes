#pragma once

#include <string>
#include <vector>

namespace snotes::i18n
{

/// A language the program can show its texts in.
struct Language
{
    std::string code; ///< ISO 639-1, e.g. "de"; the name of its file without ".txt"
    std::string name; ///< in the language itself, e.g. "Deutsch"
};

/// An entry of the menu "Help > Language".
struct LanguageChoice
{
    std::string label; ///< in the current language for the system language, otherwise the language's own name
    std::string setting; ///< value to save, LanguageCatalog::kSystemLanguage or a language code
    bool ticked = false; ///< the saved setting
};

/// The languages of the program (AP0, F-UI-01 to 03): English, the language of the texts in the code, and
/// one translation file per further language in the format of juce::LocalisedStrings
/// (resources/Languages/<code>.txt). A new language needs only a new file. Texts a file does not translate
/// stay English.
class LanguageCatalog
{
public:
    static constexpr const char* kEnglish = "en";
    /// Setting value for "follow the language of the operating system", the default.
    static constexpr const char* kSystemLanguage = "";

    /// Adds the translation file of the language `code`. Its line "language:" names the language.
    void add(const std::string& code, const std::string& fileText);

    /// English first, then the added languages in the order they were added.
    [[nodiscard]] const std::vector<Language>& languages() const noexcept { return languages_; }
    [[nodiscard]] bool contains(const std::string& code) const;

    /// The language to show: the setting if it names a known language, otherwise the system language
    /// (e.g. "de" or "de_DE") if it is known, otherwise English.
    [[nodiscard]] std::string resolve(const std::string& setting, const std::string& systemLanguage) const;

    /// "System language (Deutsch)", then every language; the saved setting is ticked, the system language
    /// also for a setting that names no known language.
    [[nodiscard]] std::vector<LanguageChoice> choices(const std::string& setting,
                                                      const std::string& systemLanguage) const;

    /// Notice after choosing a language, in the language the new setting shows: it applies after a restart.
    [[nodiscard]] std::string restartNotice(const std::string& newSetting,
                                            const std::string& systemLanguage) const;

    /// Makes `code` the current language of i18n::tr() and of JUCE's own texts; English if unknown.
    void apply(const std::string& code) const;

    /// The English text in the language `code` without changing the current language, e.g. for the notice
    /// that a newly chosen language shows after a restart.
    [[nodiscard]] std::string translate(const std::string& code, const char* english) const;

private:
    [[nodiscard]] const std::string* fileTextOf(const std::string& code) const;
    [[nodiscard]] const std::string& nameOf(const std::string& code) const;

    std::vector<Language> languages_{{kEnglish, "English"}};
    std::vector<std::string> fileTexts_; ///< per entry of languages_ after English
};

} // namespace snotes::i18n
