#include "model/TreeNode.h"

#include <utility>

namespace snotes::model
{

TreeNode::TreeNode(juce::ValueTree tree, juce::UndoManager* undoManager)
    : tree_(std::move(tree)), undoManager_(undoManager)
{
}

// Values loaded from a .dpp file arrive as text; juce::var converts them on read.

int TreeNode::getInt(const juce::Identifier& property, int fallback) const
{
    return static_cast<int>(tree_.getProperty(property, fallback));
}

std::int64_t TreeNode::getInt64(const juce::Identifier& property, std::int64_t fallback) const
{
    return static_cast<juce::int64>(tree_.getProperty(property, static_cast<juce::int64>(fallback)));
}

double TreeNode::getDouble(const juce::Identifier& property, double fallback) const
{
    return static_cast<double>(tree_.getProperty(property, fallback));
}

std::string TreeNode::getString(const juce::Identifier& property) const
{
    return tree_.getProperty(property).toString().toStdString();
}

void TreeNode::set(const juce::Identifier& property, const juce::var& value)
{
    tree_.setProperty(property, value, undoManager_);
}

void TreeNode::setString(const juce::Identifier& property, const std::string& value)
{
    set(property, juce::String::fromUTF8(value.c_str()));
}

} // namespace snotes::model
