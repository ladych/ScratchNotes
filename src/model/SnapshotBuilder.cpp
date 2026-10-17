#include "model/SnapshotBuilder.h"

namespace snotes::model
{

std::unique_ptr<const engine::ProjectSnapshot> SnapshotBuilder::build(const Project& project)
{
    auto snapshot = std::make_unique<engine::ProjectSnapshot>();
    snapshot->sampleRate = project.sampleRate();
    snapshot->masterGain = static_cast<float>(project.mix().masterGain());

    bool anySolo = false;
    for (int index = 0; index < project.numVoices(); ++index)
        if (project.voice(index).isSolo())
            anySolo = true;

    for (int index = 0; index < project.numVoices(); ++index)
    {
        const auto voice = project.voice(index);
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
        }
        snapshot->voices.push_back(entry);
    }
    return snapshot;
}

} // namespace snotes::model
