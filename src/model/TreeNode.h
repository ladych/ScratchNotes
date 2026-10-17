#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <cstdint>
#include <string>

namespace snotes::model
{

/// Base of the typed model wrappers. A wrapper is a cheap handle onto one node of the project
/// ValueTree; copies refer to the same node. Every change goes through the UndoManager given at
/// construction (F-PJ-04); nullptr makes changes without undo.
class TreeNode
{
public:
    [[nodiscard]] const juce::ValueTree& tree() const { return tree_; }
    [[nodiscard]] juce::UndoManager* undoManager() const { return undoManager_; }

protected:
    TreeNode(juce::ValueTree tree, juce::UndoManager* undoManager);

    [[nodiscard]] juce::ValueTree& mutableTree() { return tree_; }

    [[nodiscard]] int getInt(const juce::Identifier& property, int fallback) const;
    [[nodiscard]] std::int64_t getInt64(const juce::Identifier& property, std::int64_t fallback) const;
    [[nodiscard]] double getDouble(const juce::Identifier& property, double fallback) const;
    [[nodiscard]] std::string getString(const juce::Identifier& property) const;

    void set(const juce::Identifier& property, const juce::var& value);
    void setString(const juce::Identifier& property, const std::string& value);

private:
    juce::ValueTree tree_;
    juce::UndoManager* undoManager_;
};

} // namespace snotes::model
