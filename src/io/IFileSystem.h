#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace snotes::io
{

/// File access behind an interface so logic can be tested without touching disk (E-06).
class IFileSystem
{
public:
    virtual ~IFileSystem() = default;

    [[nodiscard]] virtual std::optional<std::string> readText(const std::filesystem::path& file) const = 0;
    virtual bool writeText(const std::filesystem::path& file, const std::string& text) = 0;
    virtual bool createDirectories(const std::filesystem::path& directory) = 0;
    [[nodiscard]] virtual bool exists(const std::filesystem::path& file) const = 0;
    /// Replaces an existing target file.
    virtual bool rename(const std::filesystem::path& from, const std::filesystem::path& to) = 0;

protected:
    IFileSystem() = default;
    IFileSystem(const IFileSystem&) = default;
    IFileSystem(IFileSystem&&) = default;
    IFileSystem& operator=(const IFileSystem&) = default;
    IFileSystem& operator=(IFileSystem&&) = default;
};

} // namespace snotes::io
