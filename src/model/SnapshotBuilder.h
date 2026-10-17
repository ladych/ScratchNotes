#pragma once

#include "engine/ProjectSnapshot.h"
#include "model/Project.h"

#include <memory>

namespace snotes::model
{

/// Converts the project model into the snapshot the audio thread reads.
class SnapshotBuilder
{
public:
    [[nodiscard]] static std::unique_ptr<const engine::ProjectSnapshot> build(const Project& project);
};

} // namespace snotes::model
