#pragma once

#include "engine/ProjectSnapshot.h"
#include "engine/SnapshotExchange.h"
#include "model/TreeChangeListener.h"

#include <juce_data_structures/juce_data_structures.h>

namespace snotes::model
{

using ProjectSnapshotExchange = engine::SnapshotExchange<engine::ProjectSnapshot>;

/// The single connection from the project model to the engine: on construction and after every change
/// of the project tree it publishes a new snapshot for the audio thread, so loading, editing or undoing
/// changes what is heard. Runs on the GUI thread.
class SnapshotPublisher
{
public:
    SnapshotPublisher(juce::ValueTree project, ProjectSnapshotExchange& exchange);

    void publish();

private:
    juce::ValueTree project_;
    ProjectSnapshotExchange& exchange_;
    TreeChangeListener listener_;
};

} // namespace snotes::model
