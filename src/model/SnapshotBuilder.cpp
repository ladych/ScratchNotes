#include "model/SnapshotBuilder.h"

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
    entry.monitored = voice.isMonitored();
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

} // namespace

std::unique_ptr<const engine::ProjectSnapshot> SnapshotBuilder::build(const Project& project,
                                                                      const TakeAudioSource& audio)
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
    return snapshot;
}

} // namespace snotes::model
