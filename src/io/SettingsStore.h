#pragma once

#include "io/IFileSystem.h"

#include <filesystem>
#include <optional>
#include <string>

namespace snotes::io
{

/// Persists one program setting as a text file between runs: the audio device state (F-AO-01, JUCE's
/// AudioDeviceManager XML), the language and the colour scheme (F-UI-02, F-UI-05). The text is opaque to this
/// class.
class SettingsStore
{
public:
    SettingsStore(IFileSystem& fileSystem, std::filesystem::path settingsFile);

    /// Returns the last saved state, or nothing if none was saved yet.
    [[nodiscard]] std::optional<std::string> load() const;

    /// Creates the settings directory if needed; returns false if writing failed.
    bool save(const std::string& state);

private:
    IFileSystem& fileSystem_;
    std::filesystem::path settingsFile_;
};

} // namespace snotes::io
