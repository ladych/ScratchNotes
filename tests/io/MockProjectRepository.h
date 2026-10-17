#pragma once

#include "io/IProjectRepository.h"

#include <gmock/gmock.h>

namespace snotes::io
{

class MockProjectRepository : public IProjectRepository
{
public:
    MOCK_METHOD(LoadResult, load, (const std::filesystem::path& folder), (override));
    MOCK_METHOD(ProjectFileError,
                save,
                (const juce::ValueTree& project, const std::filesystem::path& folder),
                (override));
    MOCK_METHOD(ProjectFileError,
                saveCopy,
                (const juce::ValueTree& project,
                 const std::filesystem::path& from,
                 const std::filesystem::path& to),
                (override));
    MOCK_METHOD(std::filesystem::path, freeFolder, (const std::filesystem::path& wanted), (const, override));
};

} // namespace snotes::io
