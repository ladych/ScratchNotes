#include "i18n/Translate.h"

#include <array>
#include <cstddef>

namespace snotes::i18n
{

std::string tr(const char* english)
{
    return trJuce(english).toStdString();
}

std::string tr(const char* english, std::string_view first)
{
    const std::array arguments{first};
    return substitute(tr(english), arguments);
}

std::string tr(const char* english, std::string_view first, std::string_view second)
{
    const std::array arguments{first, second};
    return substitute(tr(english), arguments);
}

std::string tr(const char* english, std::string_view first, std::string_view second, std::string_view third)
{
    const std::array arguments{first, second, third};
    return substitute(tr(english), arguments);
}

std::string tr(const char* english,
               std::string_view first,
               std::string_view second,
               std::string_view third,
               std::string_view fourth)
{
    const std::array arguments{first, second, third, fourth};
    return substitute(tr(english), arguments);
}

juce::String trJuce(const char* english)
{
    return juce::translate(juce::String::fromUTF8(english));
}

std::string decimalSeparator()
{
    // A key of its own, not ".", so no other text is translated by accident.
    return juce::translate(marked("decimal separator"), ".").toStdString();
}

std::string substitute(std::string_view text, std::span<const std::string_view> arguments)
{
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        const char next = i + 1 < text.size() ? text[i + 1] : '\0';
        const auto index = static_cast<std::size_t>(next - '1');
        if (text[i] == '%' && next >= '1' && next <= '9' && index < arguments.size())
        {
            result += arguments[index];
            ++i;
            continue;
        }
        result += text[i];
    }
    return result;
}

} // namespace snotes::i18n
