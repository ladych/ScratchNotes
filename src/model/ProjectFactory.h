#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <string>

namespace snotes::model
{

/// Creates new projects (F-PJ-01): 48 kHz, no voices yet, an empty beat grid and a mix at unity gain.
class ProjectFactory
{
public:
    static constexpr double kDefaultSampleRate = 48000.0;

    [[nodiscard]] static juce::ValueTree createDefault(const std::string& name);
    /// Adds the sections a project file may lack, e.g. one written before a section existed.
    static void addMissingSections(juce::ValueTree& project);
};

} // namespace snotes::model
