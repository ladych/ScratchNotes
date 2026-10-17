#pragma once

#include "io/IFileSystem.h"

namespace snotes::io
{

/// IFileSystem on top of std::filesystem and file streams.
class StdFileSystem final : public IFileSystem
{
public:
    [[nodiscard]] std::optional<std::string> readText(const std::filesystem::path& file) const override;
    bool writeText(const std::filesystem::path& file, const std::string& text) override;
    bool createDirectories(const std::filesystem::path& directory) override;
    [[nodiscard]] bool exists(const std::filesystem::path& file) const override;
    bool rename(const std::filesystem::path& from, const std::filesystem::path& to) override;
    bool copyFile(const std::filesystem::path& from, const std::filesystem::path& to) override;
};

} // namespace snotes::io
