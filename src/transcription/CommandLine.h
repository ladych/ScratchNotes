#pragma once

#include <optional>
#include <string>
#include <vector>

namespace snotes::transcription
{

inline constexpr const char* kDefaultProfile = "guitar6-e";

/// What scratchnotes-transcribe should do (Pflichtenheft 3, tools/).
struct CommandLineOptions
{
    std::string profileId = kDefaultProfile;
    /// Folder for the results; empty writes them next to each recording.
    std::string outputFolder;
    std::vector<std::string> recordings;
};

struct ParsedCommandLine
{
    /// Empty if the arguments are wrong or help was asked for; then message says why.
    std::optional<CommandLineOptions> options;
    std::string message;
};

/// Parses the arguments after the program name: [--profile ID] [--out FOLDER] RECORDING...
[[nodiscard]] ParsedCommandLine parseCommandLine(const std::vector<std::string>& arguments);

/// Help text with the options and the instrument profiles.
[[nodiscard]] std::string commandLineUsage();

} // namespace snotes::transcription
