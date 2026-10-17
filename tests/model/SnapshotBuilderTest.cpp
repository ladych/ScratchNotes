#include "model/SnapshotBuilder.h"

#include "model/ProjectFactory.h"
#include "model/SnapshotPublisher.h"

#include <gtest/gtest.h>

namespace snotes::model
{
namespace
{

class SnapshotBuilderTest : public ::testing::Test
{
protected:
    Voice addVoice(const std::string& id, int input)
    {
        return project.addVoice(
            {.id = id, .name = id, .profileId = "guitar6-e", .inputDi = input, .inputWet = kNoInput});
    }

    juce::ValueTree tree = ProjectFactory::createDefault("Notizen");
    Project project{tree, nullptr};
};

TEST_F(SnapshotBuilderTest, FWI03_CopiesVoiceAndMixSettings)
{
    auto voice = addVoice("v1", 2);
    voice.setInputWet(3);
    voice.setGain(0.5);
    voice.setMonitored(true);
    project.mix().setMasterGain(2.0);

    const auto snapshot = SnapshotBuilder::build(project);

    EXPECT_DOUBLE_EQ(snapshot->sampleRate, 48000.0);
    EXPECT_FLOAT_EQ(snapshot->masterGain, 2.0F);
    ASSERT_EQ(snapshot->voices.size(), 1U);
    const auto& entry = snapshot->voices[0];
    EXPECT_EQ(entry.inputDi, 2);
    EXPECT_EQ(entry.inputWet, 3);
    EXPECT_FLOAT_EQ(entry.gain, 0.5F);
    EXPECT_TRUE(entry.monitored);
    EXPECT_TRUE(entry.audible);
    EXPECT_FALSE(entry.hasTake);
}

TEST_F(SnapshotBuilderTest, FAU04_CarriesThePositionOfTheActiveTake)
{
    addVoice("v1", 0).addTake(
        {.id = "t1", .diFile = "audio/t1.wav", .startSample = 48000, .latencyOffsetSamples = 240});

    const auto snapshot = SnapshotBuilder::build(project);

    EXPECT_TRUE(snapshot->voices[0].hasTake);
    EXPECT_EQ(snapshot->voices[0].takeStartSample, 48000);
    EXPECT_EQ(snapshot->voices[0].takeLatencyOffsetSamples, 240);
}

TEST_F(SnapshotBuilderTest, FWI03_MutedVoiceIsSilent)
{
    addVoice("v1", 0).setMuted(true);
    addVoice("v2", 1);

    const auto snapshot = SnapshotBuilder::build(project);

    EXPECT_FALSE(snapshot->voices[0].audible);
    EXPECT_TRUE(snapshot->voices[1].audible);
}

TEST_F(SnapshotBuilderTest, FWI03_SoloSilencesTheOtherVoices)
{
    addVoice("v1", 0);
    addVoice("v2", 1).setSolo(true);
    auto mutedSolo = addVoice("v3", 2);
    mutedSolo.setSolo(true);
    mutedSolo.setMuted(true);

    const auto snapshot = SnapshotBuilder::build(project);

    EXPECT_FALSE(snapshot->voices[0].audible);
    EXPECT_TRUE(snapshot->voices[1].audible);
    EXPECT_FALSE(snapshot->voices[2].audible);
}

TEST(SnapshotPublisherTest, FPJ04_PublishesOnStartAndAfterEveryChange)
{
    juce::UndoManager undoManager;
    auto tree = ProjectFactory::createDefault("Notizen");
    ProjectSnapshotExchange exchange;
    const SnapshotPublisher publisher{tree, exchange};

    ASSERT_NE(exchange.acquire(), nullptr);
    EXPECT_TRUE(exchange.acquire()->voices.empty());

    Project project{tree, &undoManager};
    project.addVoice({.id = "v1", .name = "Gitarre", .profileId = "guitar6-e", .inputDi = 0});
    EXPECT_EQ(exchange.acquire()->voices.size(), 1U);

    undoManager.undo();
    EXPECT_TRUE(exchange.acquire()->voices.empty());
}

} // namespace
} // namespace snotes::model
