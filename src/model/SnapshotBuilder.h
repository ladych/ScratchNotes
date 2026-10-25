#pragma once

#include "engine/ProjectSnapshot.h"
#include "model/Project.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace snotes::model
{

/// Returns the samples of a take file given relative to the project folder, or null if it cannot be read.
using TakeAudioSource = std::function<std::shared_ptr<const engine::TakeAudio>(const std::string& file)>;

/// The notes of a voice in the score as sound (F-NA-05), by the voice's name.
struct ScoreVoiceNotes
{
    std::string name;
    std::shared_ptr<const engine::SynthNotes> notes;
};

/// Returns the notes of the voices in the score, in the order of the score.
using ScoreNotesSource = std::function<std::vector<ScoreVoiceNotes>()>;

/// Converts the project model into the snapshot the audio thread reads. While the project plays the
/// transcription, every voice gets the notes of the voice in the score with its name, in turn if several
/// share it.
class SnapshotBuilder
{
public:
    [[nodiscard]] static std::unique_ptr<const engine::ProjectSnapshot>
    build(const Project& project, const TakeAudioSource& audio, const ScoreNotesSource& notes);
};

} // namespace snotes::model
