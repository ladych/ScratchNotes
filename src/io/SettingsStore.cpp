#include "io/SettingsStore.h"

#include <utility>

namespace snotes::io
{

SettingsStore::SettingsStore(IFileSystem& fileSystem, std::filesystem::path settingsFile)
    : fileSystem_(fileSystem), settingsFile_(std::move(settingsFile))
{
}

std::optional<std::string> SettingsStore::load() const
{
    auto state = fileSystem_.readText(settingsFile_);
    if (!state || state->empty())
        return std::nullopt;
    return state;
}

bool SettingsStore::save(const std::string& state)
{
    if (!fileSystem_.createDirectories(settingsFile_.parent_path()))
        return false;
    return fileSystem_.writeText(settingsFile_, state);
}

} // namespace snotes::io
