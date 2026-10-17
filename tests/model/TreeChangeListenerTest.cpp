#include "model/TreeChangeListener.h"

#include <gtest/gtest.h>

#include <memory>

namespace snotes::model
{
namespace
{

const juce::Identifier kRoot{"ROOT"};
const juce::Identifier kChild{"CHILD"};
const juce::Identifier kValue{"value"};

class TreeChangeListenerTest : public ::testing::Test
{
protected:
    juce::ValueTree root{kRoot};
    int changes = 0;
    std::unique_ptr<TreeChangeListener> listener =
        std::make_unique<TreeChangeListener>(root, [this] { ++changes; });
};

TEST_F(TreeChangeListenerTest, FPJ02_ReportsPropertyChanges)
{
    root.setProperty(kValue, 1, nullptr);

    EXPECT_EQ(changes, 1);
}

TEST_F(TreeChangeListenerTest, FPJ02_ReportsChangesDeepInTheTree)
{
    juce::ValueTree child{kChild};
    root.appendChild(child, nullptr);

    child.setProperty(kValue, 1, nullptr);

    EXPECT_EQ(changes, 2);
}

TEST_F(TreeChangeListenerTest, FPJ02_ReportsAddedRemovedAndReorderedChildren)
{
    root.appendChild(juce::ValueTree{kChild}, nullptr);
    root.appendChild(juce::ValueTree{kChild}, nullptr);
    root.moveChild(0, 1, nullptr);
    root.removeChild(0, nullptr);

    EXPECT_EQ(changes, 4);
}

TEST_F(TreeChangeListenerTest, FPJ02_StopsReportingWhenDestroyed)
{
    listener.reset();

    root.setProperty(kValue, 1, nullptr);

    EXPECT_EQ(changes, 0);
}

} // namespace
} // namespace snotes::model
