#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <functional>

namespace snotes::model
{

/// Calls one function for every change anywhere in a ValueTree: property changes and added,
/// removed or reordered children. Detaches itself on destruction.
class TreeChangeListener final : private juce::ValueTree::Listener
{
public:
    TreeChangeListener(juce::ValueTree tree, std::function<void()> onChange);
    ~TreeChangeListener() override;

    TreeChangeListener(const TreeChangeListener&) = delete;
    TreeChangeListener& operator=(const TreeChangeListener&) = delete;
    TreeChangeListener(TreeChangeListener&&) = delete;
    TreeChangeListener& operator=(TreeChangeListener&&) = delete;

private:
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int index) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;

    juce::ValueTree tree_;
    std::function<void()> onChange_;
};

} // namespace snotes::model
