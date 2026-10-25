#pragma once

#include "engine/MonitorSource.h"
#include "engine/NoteSynth.h"
#include "engine/TakeAudio.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace snotes::engine
{

/// What the audio thread needs of one voice. Built on the GUI thread and read-only afterwards.
struct VoiceSnapshot
{
    /// Input channels, -1 for none.
    int inputDi = -1;
    int inputWet = -1;
    float gain = 1.0F;
    /// False if muted, or if another voice is solo and this one is not.
    bool audible = true;
    MonitorSource monitor = MonitorSource::off;
    bool hasTake = false;
    std::int64_t takeStartSample = 0;
    std::int64_t takeLatencyOffsetSamples = 0;
    /// The channel of the active take that plays: processed if recorded, else DI (F-WI-02); null if the
    /// voice has no take or its file is missing. Freed with the snapshot on the GUI thread.
    std::shared_ptr<const TakeAudio> audio;
    /// The voice's notes in the score as sound (F-NA-05), null if it has none; they play instead of the take
    /// while the snapshot plays the transcription.
    std::shared_ptr<const SynthNotes> notes;
};

/// Immutable state of the project for the audio thread (Pflichtenheft chapter 4), handed over through
/// SnapshotExchange.
struct ProjectSnapshot
{
    double sampleRate = 0.0;
    float masterGain = 1.0F;
    /// Click tempo in beats per minute, 0 while the click is off (F-WI-07).
    double clickBpm = 0.0;
    /// The transcription plays instead of the takes, to compare (F-NA-05).
    bool playsTranscription = false;
    std::vector<VoiceSnapshot> voices;
};

} // namespace snotes::engine
