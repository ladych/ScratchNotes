#include "model/SnapshotBuilder.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace snotes::model
{
namespace
{

engine::VoiceSnapshot voiceSnapshot(const Voice& voice, bool anySolo, const TakeAudioSource& audio)
{
    engine::VoiceSnapshot entry;
    entry.inputDi = voice.inputDi();
    entry.inputWet = voice.inputWet();
    entry.gain = static_cast<float>(voice.gain());
    entry.audible = !voice.isMuted() && (!anySolo || voice.isSolo());
    entry.monitor = voice.monitor();
    if (const auto take = voice.activeTake())
    {
        entry.hasTake = true;
        entry.takeStartSample = take->startSample();
        entry.takeLatencyOffsetSamples = take->latencyOffsetSamples();
        const auto wet = take->wetFile();
        entry.audio = audio(wet.empty() ? take->diFile() : wet);
    }
    return entry;
}

/// Gives every voice the first notes left with its name.
void addNotes(engine::ProjectSnapshot& snapshot, const Project& project, std::vector<ScoreVoiceNotes> notes)
{
    for (int index = 0; index < project.numVoices(); ++index)
    {
        const auto name = project.voice(index).name();
        for (auto& entry : notes)
            if (entry.notes && entry.name == name)
            {
                snapshot.voices[static_cast<std::size_t>(index)].notes = std::move(entry.notes);
                break;
            }
    }
}

} // namespace

std::unique_ptr<const engine::ProjectSnapshot>
SnapshotBuilder::build(const Project& project, const TakeAudioSource& audio, const ScoreNotesSource& notes)
{
    auto snapshot = std::make_unique<engine::ProjectSnapshot>();
    snapshot->sampleRate = project.sampleRate();
    const auto mix = project.mix();
    snapshot->masterGain = static_cast<float>(mix.masterGain());
    if (mix.isClickOn())
        snapshot->clickBpm = mix.clickBpm();

    bool anySolo = false;
    for (int index = 0; index < project.numVoices(); ++index)
        if (project.voice(index).isSolo())
            anySolo = true;

    for (int index = 0; index < project.numVoices(); ++index)
        snapshot->voices.push_back(voiceSnapshot(project.voice(index), anySolo, audio));
    if (project.playback() == Project::Playback::transcription)
    {
        snapshot->playsTranscription = true;
        addNotes(*snapshot, project, notes());
    }
    return snapshot;
}

} // namespace snotes::model
