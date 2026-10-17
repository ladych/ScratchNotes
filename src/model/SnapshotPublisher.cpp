#include "model/SnapshotPublisher.h"

#include "model/Project.h"
#include "model/SnapshotBuilder.h"

#include <utility>

namespace snotes::model
{

SnapshotPublisher::SnapshotPublisher(juce::ValueTree project, ProjectSnapshotExchange& exchange)
    : project_(std::move(project)), exchange_(exchange), listener_(project_, [this] { publish(); })
{
    publish();
}

void SnapshotPublisher::publish()
{
    exchange_.publish(SnapshotBuilder::build(Project{project_, nullptr}));
}

} // namespace snotes::model
