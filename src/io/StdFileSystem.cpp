#include "io/StdFileSystem.h"

#include <cstring>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

namespace snotes::io
{
namespace
{

class StdBinaryFile final : public IBinaryFile
{
public:
    explicit StdBinaryFile(std::fstream stream) : stream_(std::move(stream)) {}

    bool append(std::span<const std::byte> bytes) override
    {
        stream_.seekp(0, std::ios::end);
        return write(bytes);
    }

    bool overwrite(std::uint64_t offset, std::span<const std::byte> bytes) override
    {
        stream_.seekp(static_cast<std::streamoff>(offset));
        return write(bytes);
    }

    bool flush() override { return static_cast<bool>(stream_.flush()); }

private:
    bool write(std::span<const std::byte> bytes)
    {
        // std::fstream writes chars; copy the bytes instead of reinterpreting the pointer.
        std::vector<char> chars(bytes.size());
        std::memcpy(chars.data(), bytes.data(), bytes.size());
        stream_.write(chars.data(), static_cast<std::streamsize>(chars.size()));
        return static_cast<bool>(stream_);
    }

    std::fstream stream_;
};

} // namespace

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

bool StdFileSystem::copyFile(const std::filesystem::path& from, const std::filesystem::path& to)
{
    std::error_code error;
    std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, error);
    return !error;
}

std::unique_ptr<IBinaryFile> StdFileSystem::createBinaryFile(const std::filesystem::path& file)
{
    std::fstream stream{file, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc};
    if (!stream)
        return nullptr;
    return std::make_unique<StdBinaryFile>(std::move(stream));
}

} // namespace snotes::io
