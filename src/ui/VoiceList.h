#pragma once

#include "model/IIdGenerator.h"
#include "model/Project.h"

namespace snotes::ui
{

/// Adds the voice "Voice N" (F-AU-01) with the default instrument profile, recording its DI signal from
/// the first of the `numInputs` inputs no other voice uses, or from no input if all are taken. Starts a
/// new undo step.
model::Voice addVoice(model::Project& project, model::IIdGenerator& idGenerator, int numInputs);

} // namespace snotes::ui
