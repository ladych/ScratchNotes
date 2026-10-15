#pragma once

#include "io/IFileSystem.h"

#include <gmock/gmock.h>

namespace snotes::io
{

class MockFileSystem : public IFileSystem
{
public:
    MOCK_METHOD(std::optional<std::string>, readText, (const std::filesystem::path& file), (const, override));
    MOCK_METHOD(bool, writeText, (const std::filesystem::path& file, const std::string& text), (override));
    MOCK_METHOD(bool, createDirectories, (const std::filesystem::path& directory), (override));
    MOCK_METHOD(bool, exists, (const std::filesystem::path& file), (const, override));
    MOCK_METHOD(bool,
                rename,
                (const std::filesystem::path& from, const std::filesystem::path& to),
                (override));
};

} // namespace snotes::io
