#include "ui/TrackList.h"

#include "model/ModelIds.h"
#include "model/ProjectFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace snotes::ui
{
namespace
{

using ::testing::ElementsAre;

class TrackListTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto guitar = project.addVoice(
            {.id = "g", .name = "Gitarre 1", .profileId = "guitar7-gs", .inputDi = 0, .inputWet = 1});
        guitar.addTake({.id = "t1", .diFile = "audio/a DI.wav", .recordedAt = "2026-10-17T09:40:00"});
        guitar.addTake({.id = "t2",
                        .diFile = "audio/b DI.wav",
                        .wetFile = "audio/b bearbeitet.wav",
                        .recordedAt = "2026-10-17T09:45:12"});
        project.addVoice({.id = "b", .name = "Bass", .profileId = "unknown", .inputDi = 2});
        undoManager.clearUndoHistory();
    }

    [[nodiscard]] std::vector<TrackRow> rows() const
    {
        return trackRows(project, [](const std::string& id) { return id == "b"; });
    }

    juce::UndoManager undoManager;
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, &undoManager};
    TrackEditor editor{project};
};

TEST_F(TrackListTest, FAU01_ProfileChoicesAreTranslated)
{
    EXPECT_THAT(instrumentProfileChoices(),
                ElementsAre("Gitarre 6 Saiten, E-Standard",
                            "Gitarre 7 Saiten, Gis-Standard",
                            "Bass 4 Saiten, E-Standard",
                            "Bass 5 Saiten, Gis-Standard"));
}

TEST_F(TrackListTest, FWI06_MonitorChoicesFollowTheMonitorSources)
{
    EXPECT_THAT(monitorChoices(), ElementsAre("Monitor aus", "Monitor bearbeitet", "Monitor DI"));
}

TEST(VisibleSecondsTest, FWI01_ShowsAtLeastHalfAMinute)
{
    EXPECT_DOUBLE_EQ(visibleSeconds(0.0, 0.0), 30.0);
    EXPECT_DOUBLE_EQ(visibleSeconds(24.0, 3.0), 30.0);
}

TEST(VisibleSecondsTest, FWI01_GrowsWithTheTakesAndThePosition)
{
    EXPECT_DOUBLE_EQ(visibleSeconds(71.0, 10.0), 80.0);
    EXPECT_DOUBLE_EQ(visibleSeconds(10.0, 95.5), 110.0);
}

TEST_F(TrackListTest, FAU01_InputChoicesStartWithNone)
{
    EXPECT_THAT(inputChoices({"Hi-Z 1", "Mic 2"}), ElementsAre("— kein —", "1 · Hi-Z 1", "2 · Mic 2"));
}

TEST_F(TrackListTest, FAU01_RowsShowTheVoiceSettings)
{
    const auto all = rows();

    ASSERT_EQ(all.size(), 2U);
    EXPECT_EQ(all[0].id, "g");
    EXPECT_EQ(all[0].name, "Gitarre 1");
    EXPECT_EQ(all[0].profile, 1);
    EXPECT_EQ(all[0].diChoice, 1);
    EXPECT_EQ(all[0].wetChoice, 2);
    EXPECT_FALSE(all[0].armed);
    EXPECT_EQ(all[0].monitor, engine::MonitorSource::off);
    EXPECT_EQ(all[1].profile, -1);
    EXPECT_EQ(all[1].wetChoice, 0);
    EXPECT_TRUE(all[1].armed);
}

TEST_F(TrackListTest, FAU04_RowsListTheTakesAndTheActiveFiles)
{
    const auto all = rows();

    EXPECT_THAT(all[0].takes, ElementsAre("Take 1 · 09:40", "Take 2 · 09:45"));
    EXPECT_EQ(all[0].activeTake, 1);
    EXPECT_EQ(all[0].diFile, "audio/b DI.wav");
    EXPECT_EQ(all[0].wetFile, "audio/b bearbeitet.wav");
    EXPECT_TRUE(all[1].takes.empty());
    EXPECT_EQ(all[1].activeTake, -1);
    EXPECT_EQ(all[1].diFile, "");
}

TEST_F(TrackListTest, FWI05_RowsPlaceTheActiveTakeAfterLatencyCompensation)
{
    auto take = project.voice(0).take(1);
    take.setStartSample(96000);
    take.setLatencyOffsetSamples(480);

    EXPECT_DOUBLE_EQ(rows()[0].takeStartSeconds, (96000.0 - 480.0) / 48000.0);
    EXPECT_DOUBLE_EQ(rows()[1].takeStartSeconds, 0.0);
}

TEST_F(TrackListTest, FPJ03_MissingAudioIsMarked)
{
    auto take = project.voice(0).take(0).tree();
    take.setProperty(model::ids::audioMissing, true, nullptr);

    EXPECT_EQ(rows()[0].takes[0], "Take 1 · 09:40 (fehlt)");
}

TEST_F(TrackListTest, FTX01_TranscribedTakesShowTheirNoteCount)
{
    project.voice(0).take(1).setNotes("audio/t2.notes.csv", 42);

    EXPECT_EQ(rows()[0].takes[1], "Take 2 · 09:45 · 42 Noten");
    EXPECT_EQ(rows()[0].diCaption, "DI · 42 Noten");
    EXPECT_EQ(rows()[1].diCaption, "DI");
}

TEST_F(TrackListTest, FAU04_ShortRecordingTimeIsShownAsIs)
{
    project.voice(1).addTake({.id = "t3", .diFile = "x.wav", .recordedAt = "heute"});

    EXPECT_EQ(rows()[1].takes[0], "Take 1 · heute");
}

TEST_F(TrackListTest, FAU04_UnknownActiveTakeSelectsNone)
{
    auto voice = project.voice(0).tree();
    voice.setProperty(model::ids::activeTakeId, "weg", nullptr);

    EXPECT_EQ(rows()[0].activeTake, -1);
}

TEST_F(TrackListTest, FAU01_EditsVoiceSettingsAsSeparateUndoSteps)
{
    editor.rename("g", "Rhythmus");
    editor.chooseProfile("g", 3);
    editor.chooseInputs("g", 3, 0);
    editor.setMuted("g", true);
    editor.setSolo("g", true);
    editor.setMonitor("g", engine::MonitorSource::processed);
    editor.setGain("g", 0.5);

    const auto voice = project.voice(0);
    EXPECT_EQ(voice.monitor(), engine::MonitorSource::processed);
    EXPECT_EQ(rows()[0].monitor, engine::MonitorSource::processed);
    EXPECT_EQ(voice.name(), "Rhythmus");
    EXPECT_EQ(voice.profileId(), "bass5-gs");
    EXPECT_EQ(voice.inputDi(), 2);
    EXPECT_EQ(voice.inputWet(), model::kNoInput);
    EXPECT_TRUE(voice.isMuted());
    EXPECT_TRUE(voice.isSolo());
    EXPECT_DOUBLE_EQ(voice.gain(), 0.5);

    undoManager.undo();
    EXPECT_DOUBLE_EQ(voice.gain(), 1.0);
    EXPECT_TRUE(voice.isSolo());
}

TEST_F(TrackListTest, FWI07_SwitchesTheClickAndSetsItsTempo)
{
    editor.setClickOn(true);
    editor.setClickBpm(95.0);

    EXPECT_TRUE(project.mix().isClickOn());
    EXPECT_DOUBLE_EQ(project.mix().clickBpm(), 95.0);
    undoManager.undo();
    EXPECT_DOUBLE_EQ(project.mix().clickBpm(), 120.0);
    EXPECT_TRUE(project.mix().isClickOn());
}

TEST_F(TrackListTest, FAU01_InvalidProfileIsIgnored)
{
    editor.chooseProfile("g", -1);
    editor.chooseProfile("g", 4);

    EXPECT_EQ(project.voice(0).profileId(), "guitar7-gs");
    EXPECT_FALSE(undoManager.canUndo());
}

TEST_F(TrackListTest, FAU04_ChoosesTakes)
{
    editor.chooseTake("g", 0);
    EXPECT_EQ(project.voice(0).activeTake()->id(), "t1");

    editor.chooseTake("g", 5);
    editor.chooseTake("g", -1);
    EXPECT_EQ(project.voice(0).activeTake()->id(), "t1");
}

TEST_F(TrackListTest, FAU04_DeletesTheActiveTake)
{
    editor.chooseTake("g", 0);

    editor.deleteActiveTake("g");

    EXPECT_EQ(project.voice(0).numTakes(), 1);
    EXPECT_EQ(project.voice(0).activeTake()->id(), "t2");
    undoManager.undo();
    EXPECT_EQ(project.voice(0).numTakes(), 2);
}

TEST_F(TrackListTest, FAU04_DeletingWithoutTakeDoesNothing)
{
    editor.deleteActiveTake("b");
    editor.deleteActiveTake("unknown");

    EXPECT_EQ(project.voice(1).numTakes(), 0);
    EXPECT_FALSE(undoManager.canUndo());
}

TEST_F(TrackListTest, FAU01_RemovesVoices)
{
    editor.removeVoice("b");
    editor.removeVoice("unknown");

    EXPECT_EQ(project.numVoices(), 1);
    undoManager.undo();
    EXPECT_EQ(project.numVoices(), 2);
}

TEST_F(TrackListTest, FAU01_EditsOfRemovedVoicesAreIgnored)
{
    editor.rename("unknown", "x");

    EXPECT_FALSE(undoManager.canUndo());
}

} // namespace
} // namespace snotes::ui
