#include "io/ProjectSerializer.h"

#include "model/ModelIds.h"
#include "model/ProjectFactory.h"

namespace snotes::io
{
namespace ids = model::ids;

std::string ProjectSerializer::toXml(const juce::ValueTree& project)
{
    auto copy = project.createCopy();
    copy.setProperty(ids::formatVersion, kFormatVersion, nullptr);
    for (auto voice : copy.getChildWithName(ids::voices))
        for (auto take : voice.getChildWithName(ids::takes))
            take.removeProperty(ids::audioMissing, nullptr);
    return copy.toXmlString().toStdString();
}

ParsedProject ProjectSerializer::fromXml(const std::string& xml)
{
    auto project = juce::ValueTree::fromXml(juce::String::fromUTF8(xml.c_str()));
    if (!project.hasType(ids::project) || !project.hasProperty(ids::formatVersion))
        return {{}, ProjectFileError::notAProject};
    if (static_cast<int>(project[ids::formatVersion]) > kFormatVersion)
        return {{}, ProjectFileError::newerFormatVersion};

    project.removeProperty(ids::formatVersion, nullptr);
    model::ProjectFactory::addMissingSections(project);
    return {project, ProjectFileError::none};
}

} // namespace snotes::io
