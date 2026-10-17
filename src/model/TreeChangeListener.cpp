#include "model/TreeChangeListener.h"

#include <utility>

namespace snotes::model
{

TreeChangeListener::TreeChangeListener(juce::ValueTree tree, std::function<void()> onChange)
    : tree_(std::move(tree)), onChange_(std::move(onChange))
{
    tree_.addListener(this);
}

TreeChangeListener::~TreeChangeListener()
{
    tree_.removeListener(this);
}

void TreeChangeListener::valueTreePropertyChanged(juce::ValueTree& /*tree*/,
                                                  const juce::Identifier& /*property*/)
{
    onChange_();
}

void TreeChangeListener::valueTreeChildAdded(juce::ValueTree& /*parent*/, juce::ValueTree& /*child*/)
{
    onChange_();
}

void TreeChangeListener::valueTreeChildRemoved(juce::ValueTree& /*parent*/,
                                               juce::ValueTree& /*child*/,
                                               int /*index*/)
{
    onChange_();
}

void TreeChangeListener::valueTreeChildOrderChanged(juce::ValueTree& /*parent*/,
                                                    int /*oldIndex*/,
                                                    int /*newIndex*/)
{
    onChange_();
}

} // namespace snotes::model
