#pragma once

#include <juce_data_structures/juce_data_structures.h>

/// Type and property names of the project ValueTree (Pflichtenheft chapter 4). They are also the
/// element and attribute names in projekt.snotes, so renaming one breaks existing project files.
namespace snotes::model::ids
{

// Tree types
inline const juce::Identifier project{"PROJECT"};
inline const juce::Identifier voices{"VOICES"};
inline const juce::Identifier voice{"VOICE"};
inline const juce::Identifier takes{"TAKES"};
inline const juce::Identifier take{"TAKE"};
inline const juce::Identifier beatGrid{"BEAT_GRID"};
inline const juce::Identifier meterReading{"METER_READING"};
inline const juce::Identifier mix{"MIX"};

// Properties
inline const juce::Identifier formatVersion{"formatVersion"};
inline const juce::Identifier name{"name"};
inline const juce::Identifier sampleRate{"sampleRate"};
inline const juce::Identifier id{"id"};
inline const juce::Identifier profileId{"profileId"};
inline const juce::Identifier inputDi{"inputDi"};
inline const juce::Identifier inputWet{"inputWet"};
inline const juce::Identifier gain{"gain"};
inline const juce::Identifier mute{"mute"};
inline const juce::Identifier solo{"solo"};
inline const juce::Identifier monitor{"monitor"};
inline const juce::Identifier activeTakeId{"activeTakeId"};
inline const juce::Identifier diFile{"diFile"};
inline const juce::Identifier wetFile{"wetFile"};
inline const juce::Identifier startSample{"startSample"};
inline const juce::Identifier latencyOffsetSamples{"latencyOffsetSamples"};
inline const juce::Identifier recordedAt{"recordedAt"};
inline const juce::Identifier notesFile{"notesFile"};
inline const juce::Identifier noteCount{"noteCount"};
inline const juce::Identifier onsetsFile{"onsetsFile"};
inline const juce::Identifier eventsFile{"eventsFile"};
inline const juce::Identifier eventCount{"eventCount"};
inline const juce::Identifier beats{"beats"};
inline const juce::Identifier downbeatPhase{"downbeatPhase"};
inline const juce::Identifier source{"source"};
inline const juce::Identifier chosenMeter{"chosenMeter"};
inline const juce::Identifier numerator{"numerator"};
inline const juce::Identifier confidence{"confidence"};
inline const juce::Identifier phase{"phase"};
inline const juce::Identifier masterGain{"masterGain"};
inline const juce::Identifier click{"click"};
inline const juce::Identifier clickBpm{"clickBpm"};

/// Runtime-only marker for a take whose audio file was missing when the project was loaded
/// (F-PJ-03). It is never written to projekt.snotes.
inline const juce::Identifier audioMissing{"audioMissing"};

} // namespace snotes::model::ids
