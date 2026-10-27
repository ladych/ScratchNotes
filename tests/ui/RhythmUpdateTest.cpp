#include "ui/RhythmUpdate.h"

#include "io/MockFileSystem.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "notation/MusicXml.h"
#include "notation/Score.h"
#include "rhythm/RhythmCsv.h"
#include "rhythm/RhythmReference.h"
#include "transcription/NoteCsv.h"
#include "ui/ScoreDisplay.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace snotes::ui
{
namespace
{

namespace fs = std::filesystem;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

/// A project folder in memory: the riff excerpt of the PoC as transcription of the bass.
class RhythmUpdateTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, readText(_))
            .WillByDefault(
                [this](const fs::path& file) -> std::optional<std::string>
                {
                    const auto found = files.find(file);
                    if (found == files.end())
                        return std::nullopt;
                    return found->second;
                });
        ON_CALL(fileSystem, writeText(_, _))
            .WillByDefault(
                [this](const fs::path& file, const std::string& text)
                {
                    files[file] = text;
                    return true;
                });
        auto bass = project.addVoice({.id = "v1", .name = "Bass", .profileId = "bass5-gs"});
        bass.addTake({.id = "t1", .diFile = "audio/Bass Take 1 DI.wav"});
        auto take = bass.addTake({.id = "t2",
                                  .diFile = "audio/Bass Take 2 DI.wav",
                                  .startSample = 48000,
                                  .latencyOffsetSamples = 480});
        take.setNotes("audio/Bass Take 2 DI.notes.csv", 105);
        take.setOnsets("audio/Bass Take 2 DI.onsets.csv");
        files[folder / "audio" / "Bass Take 2 DI.notes.csv"] =
            transcription::notesToCsv(rhythm::test::readNotes("riff"));
        files[folder / "audio" / "Bass Take 2 DI.onsets.csv"] =
            rhythm::onsetsToCsv({.beat = rhythm::test::readFloats("riff-oenv441.f32"),
                                 .detail = rhythm::test::readFloats("riff-oenv256.f32")});
        project.addVoice({.id = "v2", .name = "Leer", .profileId = "guitar6-e"});
        auto guitar = project.addVoice({.id = "v3", .name = "Gitarre", .profileId = "guitar6-e"});
        guitar.addTake({.id = "t3", .diFile = "audio/Gitarre Take 1 DI.wav"});
    }

    fs::path folder = fs::path{"projects"} / "Probe";
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, nullptr};
    NiceMock<io::MockFileSystem> fileSystem;
    std::map<fs::path, std::string> files;
};

TEST_F(RhythmUpdateTest, FTX05_StoresTheGridInSongSamplesAndTheEventsOfTheTakes)
{
    project.voice(0).take(0).setEvents("audio/Bass Take 1 DI.events.csv", 3);

    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);

    const auto grid = project.beatGrid();
    const auto expected = rhythm::test::readBeats("riff", true);
    ASSERT_EQ(grid.beats().size(), expected.size());
    // The take starts at one second, recorded 10 ms late; the tracker works in frames of 20 ms.
    EXPECT_NEAR(static_cast<double>(grid.beats().front()),
                (expected.front() + 0.99) * project.sampleRate(),
                0.02 * project.sampleRate());
    EXPECT_EQ(grid.meterReadings().size(), 5U);
    EXPECT_EQ(grid.chosenMeter(), 4);
    const auto take = project.voice(0).take(1);
    EXPECT_EQ(take.eventsFile(), "audio/Bass Take 2 DI.events.csv");
    EXPECT_EQ(take.eventCount(), 27);
    EXPECT_TRUE(files.at(folder / "audio" / "Bass Take 2 DI.events.csv").starts_with("position,duration"));
    // The events of the inactive take no longer fit the grid.
    EXPECT_EQ(project.voice(0).take(0).eventsFile(), "");
    EXPECT_EQ(beatGridSummary(tree), "Beat-Raster: 88 BPM, 4/4-Takt (68 %)");
}

TEST_F(RhythmUpdateTest, FTB01_TheEventsCarryTheTabOfTheVoicesProfile)
{
    const auto events = folder / "audio" / "Bass Take 2 DI.events.csv";
    ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);
    EXPECT_TRUE(
        files.at(events).starts_with("position,duration,seconds,weight,pitches,graces,tab,grace_tab\n"));
    EXPECT_NE(files.at(events).find(",5:"), std::string::npos);
    EXPECT_EQ(files.at(events).find(",6:"), std::string::npos);

    // An unknown profile falls back to the six-string guitar.
    project.voice(0).setProfileId("unbekannt");
    ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);
    EXPECT_NE(files.at(events).find(",6:"), std::string::npos);
}

TEST_F(RhythmUpdateTest, FTX05_NoGridWithoutReadableTranscriptions)
{
    project.beatGrid().setAnalysis({1, 2}, {{.numerator = 4, .confidence = 1.0, .phase = 0}});
    files[folder / "audio" / "Bass Take 2 DI.onsets.csv"] = "kaputt";

    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::noGrid);
    EXPECT_TRUE(project.beatGrid().beats().empty());
    EXPECT_EQ(beatGridSummary(tree), "");

    files[folder / "audio" / "Bass Take 2 DI.notes.csv"] = "start,end,pitch,amplitude\nkaputt\n";
    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::noGrid);
    files.erase(folder / "audio" / "Bass Take 2 DI.notes.csv");
    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::noGrid);
}

TEST_F(RhythmUpdateTest, FTX05_NoGridWithoutTheOnsetsFile)
{
    files.erase(folder / "audio" / "Bass Take 2 DI.onsets.csv");

    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::noGrid);
}

TEST_F(RhythmUpdateTest, FTX05_TakesWithoutOnsetsAreLeftOut)
{
    auto take = project.voice(0).take(1).tree();
    take.removeProperty(model::ids::onsetsFile, nullptr);
    files.erase(folder / "audio" / "Bass Take 2 DI.onsets.csv");

    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::noGrid);
    EXPECT_CALL(fileSystem, readText(_)).Times(0);
    project.voice(0).take(1).clearNotes();
    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::noGrid);
}

TEST_F(RhythmUpdateTest, FTX09_ReportsEventsThatCannotBeWritten)
{
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(Return(false));

    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::writeFailed);
    EXPECT_EQ(project.voice(0).take(1).eventsFile(), "");
}

TEST_F(RhythmUpdateTest, FNA02_WritesTheScoreOfAllVoicesInTheChosenMeter)
{
    ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);

    EXPECT_EQ(project.beatGrid().scoreFile(), "score.musicxml");
    const auto& score = files.at(folder / "score.musicxml");
    EXPECT_NE(score.find("<part-name>Bass</part-name>"), std::string::npos);
    EXPECT_NE(score.find("<beats>4</beats>"), std::string::npos);
    EXPECT_NE(score.find("<sign>F</sign>"), std::string::npos);
    EXPECT_NE(score.find("<staff-lines>5</staff-lines>"), std::string::npos);
    // Only voices with an analysed take are in the score.
    EXPECT_EQ(score.find("Gitarre"), std::string::npos);
}

TEST_F(RhythmUpdateTest, FEX01_WritesTheScoreAsMidiInSyncWithTheGridAndWithItsTempo)
{
    ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);

    const auto& midi = files.at(folder / "score.mid");
    juce::MemoryInputStream stream{midi.data(), midi.size(), false};
    juce::MidiFile file;
    ASSERT_TRUE(file.readFrom(stream));
    file.convertTimestampTicksToSeconds();
    ASSERT_EQ(file.getNumTracks(), 2);
    // The first note of the bass sounds after the take's start at 1 s, within the first beats of the grid.
    const auto& bass = *file.getTrack(1);
    int first = 0;
    while (!bass.getEventPointer(first)->message.isNoteOn())
        ++first;
    const auto played = bass.getEventPointer(first)->message.getTimeStamp();
    const auto beats = project.beatGrid().beats();
    ASSERT_GT(beats.size(), 4U);
    EXPECT_GT(played, 1.0);
    EXPECT_LT(played, static_cast<double>(beats[4]) / project.sampleRate());
    EXPECT_NE(files.at(folder / "score.musicxml").find("<per-minute>"), std::string::npos);
}

TEST_F(RhythmUpdateTest, FNA04_TheCursorMeetsEveryNoteOfTheScoreWhereItSounds)
{
    ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);
    const auto& midi = files.at(folder / "score.mid");
    juce::MemoryInputStream stream{midi.data(), midi.size(), false};
    juce::MidiFile file;
    ASSERT_TRUE(file.readFrom(stream));
    file.convertTimestampTicksToSeconds();
    const auto score = juce::parseXML(files.at(folder / "score.musicxml"));
    ASSERT_NE(score, nullptr);
    std::set<std::int64_t> written;
    for (const auto* part : score->getChildWithTagNameIterator("part"))
        for (const auto* bar : part->getChildWithTagNameIterator("measure"))
            for (const auto* note : bar->getChildWithTagNameIterator("note"))
                if (const auto span = notation::noteSpanOf(note->getStringAttribute("id").toStdString()))
                    written.insert(span->start);

    // Every note of the MIDI file, played at its time in the recording, is a written note, or a grace note
    // a thirty-second or more before one.
    int played = 0;
    for (const auto* event : *file.getTrack(1))
        if (event->message.isNoteOn())
        {
            const auto tick = *scoreQuarterAt(tree, event->message.getTimeStamp()) * notation::kTicksPerBeat;
            const auto next = written.lower_bound(std::llround(tick));
            ASSERT_NE(next, written.end());
            const auto before = static_cast<double>(*next) - tick;
            EXPECT_NEAR(std::remainder(before, notation::kTicksPerBeat / 8.0), 0.0, 1e-3) << tick;
            EXPECT_LT(before, notation::kTicksPerBeat / 2.0);
            ++played;
        }
    EXPECT_GT(played, 50);
}

TEST_F(RhythmUpdateTest, FNA04_NoPlaceInTheScoreWithoutIt)
{
    EXPECT_EQ(scoreQuarterAt(tree, 1.0), std::nullopt);
    project.beatGrid().setAnalysis({0, 48000}, {});
    EXPECT_EQ(scoreQuarterAt(tree, 1.0), std::nullopt);
    project.beatGrid().setScoreFile("score.musicxml");
    project.beatGrid().setScoreOrigin(1);
    EXPECT_DOUBLE_EQ(*scoreQuarterAt(tree, 1.5), 0.5);
}

TEST_F(RhythmUpdateTest, FEX01_ReportsAMidiFileThatCannotBeWritten)
{
    EXPECT_CALL(fileSystem, writeText(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, writeText(folder / "score.mid", _)).WillOnce(Return(false));

    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::writeFailed);
    EXPECT_EQ(project.beatGrid().scoreFile(), "");
}

TEST_F(RhythmUpdateTest, FNA02_ReportsAScoreThatCannotBeWritten)
{
    EXPECT_CALL(fileSystem, writeText(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem, writeText(folder / "score.musicxml", _)).WillOnce(Return(false));

    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::writeFailed);
    EXPECT_EQ(project.beatGrid().scoreFile(), "");
}

TEST_F(RhythmUpdateTest, FKO02_AnEditedGridIsKeptWhileItHasBeats)
{
    project.beatGrid().setEdited({48000, 72000, 96000, 120000}, 3, 1);

    ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);
    EXPECT_THAT(project.beatGrid().beats(), ::testing::ElementsAre(48000, 72000, 96000, 120000));
    EXPECT_TRUE(project.beatGrid().isEdited());

    // A single beat is no grid: the beats are detected again.
    project.beatGrid().setEdited({48000}, 3, 1);
    ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);
    EXPECT_FALSE(project.beatGrid().isEdited());
    EXPECT_GT(project.beatGrid().beats().size(), 4U);
}

/// The notes on the staff of notes of the score.
std::vector<notation::WrittenNote> writtenNotes(const std::string& musicXml)
{
    std::vector<notation::WrittenNote> notes;
    const std::string marker = "id=\"";
    for (auto at = musicXml.find(marker); at != std::string::npos; at = musicXml.find(marker, at + 1))
    {
        const auto start = at + marker.size();
        const auto note = notation::writtenNoteOf(musicXml.substr(start, musicXml.find('"', start) - start));
        if (note && note->staff == 1)
            notes.push_back(*note);
    }
    return notes;
}

TEST_F(RhythmUpdateTest, FKO05_CorrectionsStayWhenTheGridChanges)
{
    ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);
    const auto before = writtenNotes(files.at(folder / kScoreFile));
    ASSERT_FALSE(before.empty());
    const auto first = before.front();
    const auto second = *std::ranges::find_if(
        before, [&first](const auto& note) { return std::abs(note.seconds - first.seconds) > 1e-3; });
    auto take = project.voice(0).take(1);
    const auto count = take.eventCount();
    for (const auto& note : before)
        if (note.seconds == first.seconds)
            take.setNoteEdit({.seconds = note.seconds, .pitch = note.pitch, .deleted = true});
    take.setNoteEdit({.seconds = second.seconds, .pitch = second.pitch, .toPitch = second.pitch + 2});

    for (const auto& beats :
         {std::vector<std::int64_t>{}, std::vector<std::int64_t>{48000, 72000, 96000, 120000}})
    {
        if (!beats.empty())
            project.beatGrid().setEdited(beats, 4, 0);
        ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);
        const auto after = writtenNotes(files.at(folder / kScoreFile));
        const auto at = [&after](double seconds, int pitch)
        {
            return std::ranges::any_of(
                after, [&](const auto& note) { return note.seconds == seconds && note.pitch == pitch; });
        };
        EXPECT_FALSE(at(first.seconds, first.pitch));
        EXPECT_FALSE(at(second.seconds, second.pitch));
        EXPECT_TRUE(at(second.seconds, second.pitch + 2));
    }
    // Without notes the event goes.
    project.beatGrid().clear();
    ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);
    EXPECT_EQ(take.eventCount(), count - 1);
}

TEST_F(RhythmUpdateTest, FKO02_WithoutNotesEvenAnEditedGridGoes)
{
    project.voice(0).take(1).setNotes("", 0);
    project.beatGrid().setEdited({48000, 72000}, 3, 1);

    EXPECT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::noGrid);
    EXPECT_TRUE(project.beatGrid().beats().empty());
}

TEST_F(RhythmUpdateTest, FTX06_SummaryNeedsBeatsAndAMeter)
{
    project.beatGrid().setAnalysis({0, 24000, 48000}, {});
    EXPECT_EQ(beatGridSummary(tree), "");
    project.beatGrid().setAnalysis({0}, {{.numerator = 3, .confidence = 1.0, .phase = 0}});
    EXPECT_EQ(beatGridSummary(tree), "");
    project.beatGrid().setAnalysis({0, 24000, 48000}, {{.numerator = 3, .confidence = 0.5, .phase = 0}});
    EXPECT_EQ(beatGridSummary(tree), "Beat-Raster: 120 BPM, 3/4-Takt (50 %)");
    project.beatGrid().setEdited({0, 24000, 48000}, 4, 0);
    EXPECT_EQ(beatGridSummary(tree), "Beat-Raster: 120 BPM, 4/4-Takt, bearbeitet");
}

TEST_F(RhythmUpdateTest, FNA03_BeatLinesMarkTheFirstBeatOfEachBar)
{
    EXPECT_TRUE(beatLines(tree).empty());
    project.beatGrid().setAnalysis({0, 24000, 48000, 72000},
                                   {{.numerator = 3, .confidence = 0.5, .phase = 1}});

    EXPECT_THAT(beatLines(tree),
                ::testing::ElementsAre(BeatLine{.seconds = 0.0, .downbeat = false},
                                       BeatLine{.seconds = 0.5, .downbeat = true},
                                       BeatLine{.seconds = 1.0, .downbeat = false},
                                       BeatLine{.seconds = 1.5, .downbeat = false}));
    EXPECT_NE(BeatLine{.seconds = 0.5}, BeatLine{.seconds = 1.0});
    EXPECT_NE((BeatLine{.seconds = 0.5, .downbeat = true}), (BeatLine{.seconds = 0.5, .downbeat = false}));

    project.beatGrid().setAnalysis({0, 24000}, {});
    EXPECT_FALSE(beatLines(tree)[0].downbeat);
}

TEST(RhythmFilesTest, FTX09_FilesNextToTheDiFile)
{
    EXPECT_EQ(fileNextTo("audio/Bass Take 1 DI.wav", "events.csv"), "audio/Bass Take 1 DI.events.csv");
}

} // namespace
} // namespace snotes::ui
