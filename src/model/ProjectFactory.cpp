#include "model/ProjectFactory.h"

#include "model/ModelIds.h"

namespace snotes::model
{

juce::ValueTree ProjectFactory::createDefault(const std::string& name)
{
    juce::ValueTree tree{ids::project};
    tree.setProperty(ids::name, juce::String::fromUTF8(name.c_str()), nullptr);
    tree.setProperty(ids::sampleRate, kDefaultSampleRate, nullptr);
    addMissingSections(tree);
    return tree;
}

void ProjectFactory::addMissingSections(juce::ValueTree& project)
{
    for (const auto* type : {&ids::voices, &ids::beatGrid})
        static_cast<void>(project.getOrCreateChildWithName(*type, nullptr));
    auto mix = project.getOrCreateChildWithName(ids::mix, nullptr);
    if (!mix.hasProperty(ids::masterGain))
        mix.setProperty(ids::masterGain, 1.0, nullptr);
    for (auto voice : project.getChildWithName(ids::voices))
        static_cast<void>(voice.getOrCreateChildWithName(ids::takes, nullptr));
}

} // namespace snotes::model
