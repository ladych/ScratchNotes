#include "ui/VoiceList.h"

#include "model/FakeIdGenerator.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace snotes::ui
{
namespace
{

class VoiceListTest : public ::testing::Test
{
protected:
    juce::UndoManager undoManager;
    juce::ValueTree tree = model::ProjectFactory::createDefault("Notizen");
    model::Project project{tree, &undoManager};
    model::FakeIdGenerator idGenerator;
};

TEST_F(VoiceListTest, FAU01_AddsNumberedVoicesOnFreeInputs)
{
    const auto first = addVoice(project, idGenerator, 4);
    const auto second = addVoice(project, idGenerator, 4);

    EXPECT_EQ(first.name(), "Stimme 1");
    EXPECT_EQ(first.id(), "id-1");
    EXPECT_EQ(first.profileId(), "guitar6-e");
    EXPECT_EQ(first.inputDi(), 0);
    EXPECT_EQ(first.inputWet(), model::kNoInput);
    EXPECT_EQ(second.name(), "Stimme 2");
    EXPECT_EQ(second.inputDi(), 1);
}

TEST_F(VoiceListTest, FAU01_SkipsInputsOfOtherVoices)
{
    project.addVoice({.id = "v", .name = "Gitarre", .profileId = "guitar6-e", .inputDi = 0, .inputWet = 1});

    EXPECT_EQ(addVoice(project, idGenerator, 4).inputDi(), 2);
}

TEST_F(VoiceListTest, FAU01_VoiceWithoutFreeInputHasNone)
{
    addVoice(project, idGenerator, 1);

    EXPECT_EQ(addVoice(project, idGenerator, 1).inputDi(), model::kNoInput);
}

TEST_F(VoiceListTest, FPJ04_EachAddedVoiceIsOneUndoStep)
{
    addVoice(project, idGenerator, 4);
    addVoice(project, idGenerator, 4);

    undoManager.undo();

    EXPECT_EQ(project.numVoices(), 1);
}

TEST_F(VoiceListTest, FAU01_EntriesShowProfileAndInputs)
{
    project.addVoice({.id = "a", .name = "Gitarre", .profileId = "guitar7-gs", .inputDi = 0, .inputWet = 1});
    project.addVoice({.id = "b", .name = "Bass", .profileId = "bass4-e", .inputDi = 2});
    project.addVoice({.id = "c", .name = "Neu", .profileId = "ukulele"});

    const auto entries = voiceEntries(project);

    ASSERT_EQ(entries.size(), 3U);
    EXPECT_EQ(entries[0].name, "Gitarre");
    EXPECT_EQ(entries[0].details, "Gitarre 7 Saiten, Gis-Standard · Eingänge 1 (DI) + 2");
    EXPECT_EQ(entries[1].details, "Bass 4 Saiten, E-Standard · Eingang 3");
    EXPECT_EQ(entries[2].details, "ukulele · kein Eingang");
}

} // namespace
} // namespace snotes::ui
