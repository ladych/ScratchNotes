#include "io/StdFileSystem.h"

#include <fstream>
#include <sstream>
#include <system_error>

namespace snotes::io
{

std::optional<std::string> StdFileSystem::readText(const std::filesystem::path& file) const
{
    const std::ifstream stream{file, std::ios::binary};
    if (!stream)
        return std::nullopt;

    std::ostringstream content;
    content << stream.rdbuf();
    return content.str();
}

bool StdFileSystem::writeText(const std::filesystem::path& file, const std::string& text)
{
    std::ofstream stream{file, std::ios::binary | std::ios::trunc};
    stream << text;
    return static_cast<bool>(stream);
}

bool StdFileSystem::createDirectories(const std::filesystem::path& directory)
{
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    return !error;
}

bool StdFileSystem::exists(const std::filesystem::path& file) const
{
    std::error_code error;
    return std::filesystem::exists(file, error);
}

bool StdFileSystem::rename(const std::filesystem::path& from, const std::filesystem::path& to)
{
    std::error_code error;
    std::filesystem::rename(from, to, error);
    return !error;
}

} // namespace snotes::io
