#include "model/SnapshotBuilder.h"

#include "model/ProjectFactory.h"
#include "model/SnapshotPublisher.h"

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace snotes::model
{
namespace
{

/// Take files the fake audio source can read.
class FakeTakeFiles
{
public:
    std::shared_ptr<const engine::TakeAudio> add(const std::string& file)
    {
        auto audio = std::make_shared<const engine::TakeAudio>(std::vector<float>{0.5F});
        files_[file] = audio;
        return audio;
    }

    [[nodiscard]] TakeAudioSource source()
    {
        return [this](const std::string& file) -> std::shared_ptr<const engine::TakeAudio>
        {
            requested.push_back(file);
            const auto found = files_.find(file);
            if (found == files_.end())
                return nullptr;
            return found->second;
        };
    }

    std::vector<std::string> requested;

private:
    std::map<std::string, std::shared_ptr<const engine::TakeAudio>> files_;
};

class SnapshotBuilderTest : public ::testing::Test
{
protected:
    Voice addVoice(const std::string& id, int input)
    {
        return project.addVoice(
            {.id = id, .name = id, .profileId = "guitar6-e", .inputDi = input, .inputWet = kNoInput});
    }

    [[nodiscard]] std::unique_ptr<const engine::ProjectSnapshot> build()
    {
        return SnapshotBuilder::build(project, files.source());
    }

    juce::ValueTree tree = ProjectFactory::createDefault("Notizen");
    Project project{tree, nullptr};
    FakeTakeFiles files;
};

TEST_F(SnapshotBuilderTest, FWI03_CopiesVoiceAndMixSettings)
{
    auto voice = addVoice("v1", 2);
    voice.setInputWet(3);
    voice.setGain(0.5);
    voice.setMonitored(true);
    project.mix().setMasterGain(2.0);

    const auto snapshot = build();

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
    EXPECT_EQ(entry.audio, nullptr);
    EXPECT_DOUBLE_EQ(snapshot->clickBpm, 0.0);
}

TEST_F(SnapshotBuilderTest, FWI02_PlaysTheProcessedChannelIfRecorded)
{
    const auto wet = files.add("audio/t1 bearbeitet.wav");
    files.add("audio/t1 DI.wav");
    addVoice("v1", 0).addTake(
        {.id = "t1", .diFile = "audio/t1 DI.wav", .wetFile = "audio/t1 bearbeitet.wav"});

    EXPECT_EQ(build()->voices[0].audio, wet);
}

TEST_F(SnapshotBuilderTest, FWI02_PlaysTheDiChannelWithoutProcessedOne)
{
    const auto di = files.add("audio/t1 DI.wav");
    addVoice("v1", 0).addTake({.id = "t1", .diFile = "audio/t1 DI.wav"});

    EXPECT_EQ(build()->voices[0].audio, di);
}

TEST_F(SnapshotBuilderTest, FPJ03_MissingTakeFilePlaysNothing)
{
    addVoice("v1", 0).addTake({.id = "t1", .diFile = "audio/weg.wav"});

    const auto snapshot = build();

    EXPECT_TRUE(snapshot->voices[0].hasTake);
    EXPECT_EQ(snapshot->voices[0].audio, nullptr);
    EXPECT_EQ(files.requested, std::vector<std::string>{"audio/weg.wav"});
}

TEST_F(SnapshotBuilderTest, FWI07_CarriesTheClickTempoWhileTheClickIsOn)
{
    auto mix = project.mix();
    mix.setClickBpm(90.0);
    EXPECT_DOUBLE_EQ(build()->clickBpm, 0.0);

    mix.setClickOn(true);
    EXPECT_DOUBLE_EQ(build()->clickBpm, 90.0);
}

TEST_F(SnapshotBuilderTest, FAU04_CarriesThePositionOfTheActiveTake)
{
    addVoice("v1", 0).addTake(
        {.id = "t1", .diFile = "audio/t1.wav", .startSample = 48000, .latencyOffsetSamples = 240});

    const auto snapshot = build();

    EXPECT_TRUE(snapshot->voices[0].hasTake);
    EXPECT_EQ(snapshot->voices[0].takeStartSample, 48000);
    EXPECT_EQ(snapshot->voices[0].takeLatencyOffsetSamples, 240);
}

TEST_F(SnapshotBuilderTest, FWI03_MutedVoiceIsSilent)
{
    addVoice("v1", 0).setMuted(true);
    addVoice("v2", 1);

    const auto snapshot = build();

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

    const auto snapshot = build();

    EXPECT_FALSE(snapshot->voices[0].audible);
    EXPECT_TRUE(snapshot->voices[1].audible);
    EXPECT_FALSE(snapshot->voices[2].audible);
}

TEST(SnapshotPublisherTest, FPJ04_PublishesOnStartAndAfterEveryChange)
{
    juce::UndoManager undoManager;
    auto tree = ProjectFactory::createDefault("Notizen");
    ProjectSnapshotExchange exchange;
    FakeTakeFiles files;
    const SnapshotPublisher publisher{tree, exchange, files.source()};

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
