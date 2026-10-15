#pragma once

#include <filesystem>
#include <string>

namespace snotes::io
{

/// Paths in the model and in JUCE strings are UTF-8. std::filesystem only treats char8_t input as
/// UTF-8 on every platform (on Windows a plain std::string would be read in the ANSI code page).
inline std::filesystem::path pathFromUtf8(const std::string& utf8)
{
    return {std::u8string{utf8.begin(), utf8.end()}};
}

inline std::string utf8FromPath(const std::filesystem::path& path)
{
    const auto utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
}

inline std::string genericUtf8FromPath(const std::filesystem::path& path)
{
    const auto utf8 = path.generic_u8string();
    return {utf8.begin(), utf8.end()};
}

} // namespace snotes::io
