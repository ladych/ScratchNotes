#include "transcription/CommandLine.h"

#include "model/InstrumentProfile.h"

#include <cstddef>

namespace snotes::transcription
{
namespace
{

ParsedCommandLine failure(const std::string& message)
{
    return {.options = std::nullopt, .message = message + "\n\n" + commandLineUsage()};
}

} // namespace

std::string commandLineUsage()
{
    std::string usage =
        "Usage: scratchnotes-transcribe [--profile ID] [--out FOLDER] [--separate] RECORDING...\n"
        "Detects the notes of DI recordings and writes <name>.notes.csv and <name>.mid, the rhythm and\n"
        "tablature of each voice to <name>.events.csv and <name>.tab.txt and the beat grid and meter of all\n"
        "of them, as voices of one piece, to <first name>.beats.csv and <first name>.meter.csv, and their "
        "score\n"
        "with notes and tablature to <first name>.musicxml and as MIDI to <first name>.score.mid.\n"
        "--separate gives each recording its own grid. The profile sets the strings of the tablature.\n"
        "Profiles:\n";
    for (const auto& profile : model::instrumentProfiles())
    {
        usage += "  ";
        usage += profile.id;
        usage += profile.id == kDefaultProfile ? " (default)\n" : "\n";
    }
    return usage;
}

ParsedCommandLine parseCommandLine(const std::vector<std::string>& arguments)
{
    CommandLineOptions options;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        const auto& argument = arguments[index];
        if (argument == "--help" || argument == "-h")
            return {.options = std::nullopt, .message = commandLineUsage()};
        const bool takesValue = argument == "--profile" || argument == "--out";
        if (takesValue && index + 1 == arguments.size())
            return failure("Missing value after " + argument);
        if (argument == "--profile")
            options.profileId = arguments[++index];
        else if (argument == "--out")
            options.outputFolder = arguments[++index];
        else if (argument == "--separate")
            options.separateGrids = true;
        else if (argument.starts_with("--"))
            return failure("Unknown option " + argument);
        else
            options.recordings.push_back(argument);
    }
    if (!model::findInstrumentProfile(options.profileId))
        return failure("Unknown profile " + options.profileId);
    if (options.recordings.empty())
        return failure("No recording given");
    return {.options = options, .message = {}};
}

} // namespace snotes::transcription
