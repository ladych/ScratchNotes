#include "model/SnapshotPublisher.h"

#include "model/Project.h"
#include "model/SnapshotBuilder.h"

#include <utility>

namespace snotes::model
{

SnapshotPublisher::SnapshotPublisher(juce::ValueTree project,
                                     ProjectSnapshotExchange& exchange,
                                     TakeAudioSource audio,
                                     ScoreNotesSource notes)
    : project_(std::move(project)), exchange_(exchange), audio_(std::move(audio)), notes_(std::move(notes)),
      listener_(project_, [this] { publish(); })
{
    publish();
}

void SnapshotPublisher::publish()
{
    exchange_.publish(SnapshotBuilder::build(Project{project_, nullptr}, audio_, notes_));
}

} // namespace snotes::model
