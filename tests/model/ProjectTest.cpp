#include "model/Project.h"

#include "model/ModelIds.h"
#include "model/ProjectFactory.h"

#include <gtest/gtest.h>

namespace snotes::model
{
namespace
{

class ProjectTest : public ::testing::Test
{
protected:
    Voice addGuitar()
    {
        return project.addVoice(
            {.id = "v1", .name = "Gitarre", .profileId = "guitar6-e", .inputDi = 0, .inputWet = 1});
    }

    static TakeData takeData(const std::string& id)
    {
        return {.id = id,
                .diFile = "audio/" + id + "-di.wav",
                .wetFile = "audio/" + id + "-wet.wav",
                .startSample = 480,
                .latencyOffsetSamples = 96,
                .recordedAt = "2026-10-17T09:40:00"};
    }

    juce::UndoManager undoManager;
    juce::ValueTree tree = ProjectFactory::createDefault("Notizen");
    Project project{tree, &undoManager};
};

TEST_F(ProjectTest, FPJ01_NewProjectHasNameSampleRateAndNoVoices)
{
    EXPECT_EQ(project.name(), "Notizen");
    EXPECT_DOUBLE_EQ(project.sampleRate(), 48000.0);
    EXPECT_EQ(project.numVoices(), 0);
    EXPECT_DOUBLE_EQ(project.mix().masterGain(), 1.0);
    EXPECT_TRUE(tree.getChildWithName(ids::beatGrid).isValid());
}

TEST_F(ProjectTest, FPJ01_RenamesTheProject)
{
    project.setName("Probe Dienstag");

    EXPECT_EQ(project.name(), "Probe Dienstag");
}

TEST_F(ProjectTest, FAU01_AddedVoiceKeepsItsSettings)
{
    const auto voice = addGuitar();

    ASSERT_EQ(project.numVoices(), 1);
    EXPECT_EQ(project.voice(0).id(), "v1");
    EXPECT_EQ(voice.name(), "Gitarre");
    EXPECT_EQ(voice.profileId(), "guitar6-e");
    EXPECT_EQ(voice.inputDi(), 0);
    EXPECT_EQ(voice.inputWet(), 1);
    EXPECT_DOUBLE_EQ(voice.gain(), 1.0);
    EXPECT_FALSE(voice.isMuted());
    EXPECT_FALSE(voice.isSolo());
    EXPECT_FALSE(voice.isMonitored());
    EXPECT_EQ(voice.numTakes(), 0);
    EXPECT_FALSE(voice.activeTake().has_value());
}

TEST_F(ProjectTest, FAU01_ChangesVoiceSettings)
{
    auto voice = addGuitar();

    voice.setName("Bass");
    voice.setProfileId("bass4-e");
    voice.setInputDi(2);
    voice.setInputWet(kNoInput);
    voice.setGain(0.5);
    voice.setMuted(true);
    voice.setSolo(true);
    voice.setMonitored(true);

    EXPECT_EQ(voice.name(), "Bass");
    EXPECT_EQ(voice.profileId(), "bass4-e");
    EXPECT_EQ(voice.inputDi(), 2);
    EXPECT_EQ(voice.inputWet(), kNoInput);
    EXPECT_DOUBLE_EQ(voice.gain(), 0.5);
    EXPECT_TRUE(voice.isMuted());
    EXPECT_TRUE(voice.isSolo());
    EXPECT_TRUE(voice.isMonitored());

    voice.setMuted(false);
    voice.setSolo(false);
    voice.setMonitored(false);
    EXPECT_FALSE(voice.isMuted());
    EXPECT_FALSE(voice.isSolo());
    EXPECT_FALSE(voice.isMonitored());
}

TEST_F(ProjectTest, FWI03_GainIsClampedToPlusTwelveDecibels)
{
    auto voice = addGuitar();

    voice.setGain(10.0);
    EXPECT_DOUBLE_EQ(voice.gain(), 4.0);
    voice.setGain(-1.0);
    EXPECT_DOUBLE_EQ(voice.gain(), 0.0);

    project.mix().setMasterGain(5.0);
    EXPECT_DOUBLE_EQ(project.mix().masterGain(), 4.0);
}

TEST_F(ProjectTest, FAU01_FindsVoicesById)
{
    addGuitar();

    EXPECT_EQ(project.findVoice("v1")->name(), "Gitarre");
    EXPECT_FALSE(project.findVoice("v2").has_value());
}

TEST_F(ProjectTest, FAU01_RemovesVoices)
{
    addGuitar();

    project.removeVoice(0);

    EXPECT_EQ(project.numVoices(), 0);
}

TEST_F(ProjectTest, FAU04_NewTakeBecomesActive)
{
    auto voice = addGuitar();

    voice.addTake(takeData("t1"));
    const auto take = voice.addTake(takeData("t2"));

    EXPECT_EQ(voice.numTakes(), 2);
    EXPECT_EQ(voice.take(0).id(), "t1");
    EXPECT_EQ(voice.activeTake()->id(), "t2");
    EXPECT_EQ(take.diFile(), "audio/t2-di.wav");
    EXPECT_EQ(take.wetFile(), "audio/t2-wet.wav");
    EXPECT_EQ(take.startSample(), 480);
    EXPECT_EQ(take.latencyOffsetSamples(), 96);
    EXPECT_EQ(take.recordedAt(), "2026-10-17T09:40:00");
    EXPECT_FALSE(take.isAudioMissing());
}

TEST_F(ProjectTest, FAU04_TakeDataRoundTrips)
{
    auto voice = addGuitar();
    const auto data = takeData("t1");

    const auto read = voice.addTake(data).data();

    EXPECT_EQ(read.id, data.id);
    EXPECT_EQ(read.diFile, data.diFile);
    EXPECT_EQ(read.wetFile, data.wetFile);
    EXPECT_EQ(read.startSample, data.startSample);
    EXPECT_EQ(read.latencyOffsetSamples, data.latencyOffsetSamples);
    EXPECT_EQ(read.recordedAt, data.recordedAt);
}

TEST_F(ProjectTest, FAU04_TakeWithoutProcessedSignalHasNoWetFile)
{
    auto voice = addGuitar();
    auto data = takeData("t1");
    data.wetFile.clear();

    const auto take = voice.addTake(data);

    EXPECT_EQ(take.wetFile(), "");
    EXPECT_FALSE(take.tree().hasProperty(ids::wetFile));
}

TEST_F(ProjectTest, FAU04_MovesTakes)
{
    auto voice = addGuitar();
    auto take = voice.addTake(takeData("t1"));

    take.setStartSample(1000);
    take.setLatencyOffsetSamples(-12);

    EXPECT_EQ(take.startSample(), 1000);
    EXPECT_EQ(take.latencyOffsetSamples(), -12);
}

TEST_F(ProjectTest, FAU04_ChoosesTheActiveTake)
{
    auto voice = addGuitar();
    voice.addTake(takeData("t1"));
    voice.addTake(takeData("t2"));

    voice.setActiveTake("t1");
    EXPECT_EQ(voice.activeTake()->id(), "t1");

    voice.setActiveTake("unknown");
    EXPECT_EQ(voice.activeTake()->id(), "t1");
    EXPECT_FALSE(voice.findTake("unknown").has_value());
}

TEST_F(ProjectTest, FAU04_RemovingTheActiveTakeActivatesTheLastOne)
{
    auto voice = addGuitar();
    voice.addTake(takeData("t1"));
    voice.addTake(takeData("t2"));
    voice.addTake(takeData("t3"));
    voice.setActiveTake("t2");

    voice.removeTake(1);

    EXPECT_EQ(voice.numTakes(), 2);
    EXPECT_EQ(voice.activeTake()->id(), "t3");
}

TEST_F(ProjectTest, FAU04_RemovingAnotherTakeKeepsTheActiveOne)
{
    auto voice = addGuitar();
    voice.addTake(takeData("t1"));
    voice.addTake(takeData("t2"));

    voice.removeTake(0);

    EXPECT_EQ(voice.activeTake()->id(), "t2");
}

TEST_F(ProjectTest, FAU04_RemovingTheLastTakeLeavesNoActiveTake)
{
    auto voice = addGuitar();
    voice.addTake(takeData("t1"));

    voice.removeTake(0);

    EXPECT_FALSE(voice.activeTake().has_value());
}

TEST_F(ProjectTest, FPJ04_ChangesCanBeUndone)
{
    addGuitar();
    undoManager.beginNewTransaction();
    project.voice(0).setName("Bass");

    undoManager.undo();
    EXPECT_EQ(project.voice(0).name(), "Gitarre");

    undoManager.undo();
    EXPECT_EQ(project.numVoices(), 0);

    undoManager.redo();
    EXPECT_EQ(project.numVoices(), 1);
}

TEST_F(ProjectTest, FPJ03_ReadsTheMissingAudioMarker)
{
    auto voice = addGuitar();
    auto take = voice.addTake(takeData("t1"));
    juce::ValueTree marked = take.tree();
    marked.setProperty(ids::audioMissing, true, nullptr);

    EXPECT_TRUE(voice.take(0).isAudioMissing());
}

TEST_F(ProjectTest, FPJ02_AddsMissingSectionsToOldFiles)
{
    juce::ValueTree bare{ids::project};
    juce::ValueTree voices{ids::voices};
    voices.appendChild(juce::ValueTree{ids::voice}, nullptr);
    bare.appendChild(voices, nullptr);

    ProjectFactory::addMissingSections(bare);

    EXPECT_TRUE(bare.getChildWithName(ids::beatGrid).isValid());
    EXPECT_DOUBLE_EQ(Project(bare, nullptr).mix().masterGain(), 1.0);
    EXPECT_EQ(Project(bare, nullptr).voice(0).numTakes(), 0);
    EXPECT_TRUE(bare.getChildWithName(ids::voices).getChild(0).getChildWithName(ids::takes).isValid());
}

TEST_F(ProjectTest, FPJ02_KeepsAnExistingMasterGain)
{
    project.mix().setMasterGain(0.5);

    ProjectFactory::addMissingSections(tree);

    EXPECT_DOUBLE_EQ(project.mix().masterGain(), 0.5);
}

} // namespace
} // namespace snotes::model
