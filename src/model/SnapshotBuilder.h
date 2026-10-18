#pragma once

#include "engine/ProjectSnapshot.h"
#include "model/Project.h"

#include <functional>
#include <memory>
#include <string>

namespace snotes::model
{

/// Returns the samples of a take file given relative to the project folder, or null if it cannot be read.
using TakeAudioSource = std::function<std::shared_ptr<const engine::TakeAudio>(const std::string& file)>;

/// Converts the project model into the snapshot the audio thread reads.
class SnapshotBuilder
{
public:
    [[nodiscard]] static std::unique_ptr<const engine::ProjectSnapshot> build(const Project& project,
                                                                              const TakeAudioSource& audio);
};

} // namespace snotes::model
