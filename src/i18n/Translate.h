#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <span>
#include <string>
#include <string_view>

namespace snotes::i18n
{

/// Marks an English text for translation where it is only stored, e.g. in a table; tr() translates it
/// where it is shown. tools/check-translations.py collects the texts of tr() and marked() (F-UI-03).
constexpr const char* marked(const char* english) noexcept
{
    return english;
}

/// The English UI text (UTF-8) in the current language (LanguageCatalog::apply), or the English text
/// itself if the language has no translation for it. GUI thread only.
[[nodiscard]] std::string tr(const char* english);

/// tr() as a juce::String for JUCE widgets.
[[nodiscard]] juce::String trJuce(const char* english);

/// Decimal separator of the current language: "." in English, "," in German.
[[nodiscard]] std::string decimalSeparator();

/// Replaces %1 ... %9 in the text in one pass, so arguments that contain "%" stay as they are.
[[nodiscard]] std::string substitute(std::string_view text, std::span<const std::string_view> arguments);

/// Like tr(), then %1, %2 ... %9 are replaced by the arguments (texts) in their order. Translations may
/// change the order of the placeholders.
template <typename... Arguments>
[[nodiscard]] std::string tr(const char* english, const std::string_view first, const Arguments&... rest)
{
    const std::array<std::string_view, 1 + sizeof...(Arguments)> arguments{first, std::string_view{rest}...};
    return substitute(tr(english), arguments);
}

template <typename... Arguments>
[[nodiscard]] juce::String trJuce(const char* english, const std::string_view first, const Arguments&... rest)
{
    return juce::String::fromUTF8(tr(english, first, rest...).c_str());
}

} // namespace snotes::i18n
