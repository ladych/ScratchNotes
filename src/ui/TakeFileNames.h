#pragma once

#include "io/IFileSystem.h"

#include <filesystem>
#include <string>

namespace snotes::ui
{

/// "audio/<base> DI.wav" or "audio/<base> processed.wav" (translated), relative to the project folder.
[[nodiscard]] std::string takeAudioFile(const std::string& base, bool processed);

/// "<voice name> take N" (translated) for the next take of a voice with `numTakes` takes, with N counting on
/// from numTakes + 1 until neither of its files exists in the project folder yet. Recorded and imported takes
/// are named alike.
[[nodiscard]] std::string nextTakeBaseName(const std::string& voiceName,
                                           int numTakes,
                                           const std::filesystem::path& folder,
                                           const io::IFileSystem& fileSystem);

} // namespace snotes::ui
