#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <string>

namespace snotes::io
{

enum class ProjectFileError : std::uint8_t
{
    none,
    unreadable,
    notAProject,
    newerFormatVersion,
    writeFailed
};

struct ParsedProject
{
    juce::ValueTree project;
    ProjectFileError error = ProjectFileError::none;
};

/// Converts the project ValueTree to and from the XML text of projekt.snotes (F-PJ-02). Take paths are
/// relative to the project folder with forward slashes in the model and in the file, so a project folder
/// can be moved or opened on another operating system.
class ProjectSerializer
{
public:
    static constexpr int kFormatVersion = 1;

    [[nodiscard]] static std::string toXml(const juce::ValueTree& project);
    [[nodiscard]] static ParsedProject fromXml(const std::string& xml);
};

} // namespace snotes::io
