#include "ui/NoteCorrection.h"

#include "io/MockFileSystem.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "rhythm/RhythmCsv.h"
#include "rhythm/RhythmReference.h"
#include "transcription/NoteCsv.h"
#include "ui/RhythmUpdate.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{
namespace
{

namespace fs = std::filesystem;
using rhythm::NoteEdit;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

constexpr int kNone = NoteEdit::kNone;

/// A guitar without a take, then a bass whose take is in the score as part 0, the riff of the PoC.
class NoteCorrectionTest : public ::testing::Test
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
        project.addVoice({.id = "v1", .name = "Gitarre", .profileId = "guitar6-e"});
        auto bass = project.addVoice({.id = "v2", .name = "Bass", .profileId = "bass5-gs"});
        auto take = bass.addTake({.id = "t1", .diFile = "audio/Bass DI.wav"});
        take.setNotes("audio/Bass DI.notes.csv", 105);
        take.setOnsets("audio/Bass DI.onsets.csv");
        files[folder / "audio" / "Bass DI.notes.csv"] =
            transcription::notesToCsv(rhythm::test::readNotes("riff"));
        files[folder / "audio" / "Bass DI.onsets.csv"] =
            rhythm::onsetsToCsv({.beat = rhythm::test::readFloats("riff-oenv441.f32"),
                                 .detail = rhythm::test::readFloats("riff-oenv256.f32")});
        ASSERT_EQ(updateRhythm(tree, fileSystem, folder), RhythmUpdate::updated);
        undoManager.clearUndoHistory();
    }

    [[nodiscard]] model::Take take() const { return *project.voice(1).activeTake(); }

    fs::path folder = fs::path{"projects"} / "Probe";
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, nullptr};
    juce::UndoManager undoManager;
    NiceMock<io::MockFileSystem> fileSystem;
    std::map<fs::path, std::string> files;
    std::vector<std::string> messages;
    NoteCorrection notes{tree,
                         undoManager,
                         fileSystem,
                         {.projectFolder = [this] { return std::optional{folder}; },
                          .showMessage = [this](const std::string& title, const std::string& message)
                          { messages.push_back(title + ": " + message); }}};
    const notation::WrittenNote low{
        .span = {.start = 0, .end = 6}, .part = 0, .staff = 1, .index = 0, .pitch = 35, .seconds = 1.5};
};

TEST_F(NoteCorrectionTest, FKO04_SelectsOnlyNotesOfPartsWithATake)
{
    notes.select(low);
    EXPECT_EQ(notes.selected(), low);
    auto other = low;
    other.part = 1;
    notes.select(other);
    EXPECT_EQ(notes.selected(), std::nullopt);
    notes.select(low);
    notes.select(std::nullopt);
    EXPECT_EQ(notes.selected(), std::nullopt);
}

TEST_F(NoteCorrectionTest, FKO04_NothingHappensWithoutASelection)
{
    notes.deleteNote();
    notes.transpose(1);
    notes.setDuration(6);
    notes.setString(1);
    EXPECT_TRUE(take().noteEdits().empty());
    EXPECT_TRUE(notes.stringChoices().empty());
    EXPECT_FALSE(notes.poll());
}

TEST_F(NoteCorrectionTest, FKO04_ASelectionWithoutItsTakeDoesNothing)
{
    notes.select(low);
    take().clearEvents();
    notes.deleteNote();
    EXPECT_TRUE(take().noteEdits().empty());
    EXPECT_TRUE(notes.stringChoices().empty());
    EXPECT_FALSE(notes.durationChoices()[3].ticked);
}

TEST_F(NoteCorrectionTest, FKO04_AMovedNoteIsFoundByItsNewPitch)
{
    // Another note moved to 37 at another time and a deleted one moved there do not count.
    auto other = low;
    other.seconds = 3.0;
    notes.select(other);
    notes.transpose(2);
    auto gone = low;
    gone.pitch = 36;
    notes.select(gone);
    notes.transpose(1);
    notes.deleteNote();
    notes.select(low);
    notes.transpose(2);
    EXPECT_EQ(take().noteEdits(),
              (std::vector<NoteEdit>{{.seconds = 3.0, .pitch = 35, .toPitch = 37},
                                     {.seconds = 1.5, .pitch = 36, .deleted = true, .toPitch = 37},
                                     {.seconds = 1.5, .pitch = 35, .toPitch = 37}}));
    // Selected by its new pitch, the note moves on.
    auto moved = low;
    moved.pitch = 37;
    notes.select(moved);
    notes.transpose(1);
    EXPECT_EQ(take().noteEdits().back(), (NoteEdit{.seconds = 1.5, .pitch = 35, .toPitch = 38}));
}

TEST_F(NoteCorrectionTest, FKO04_ChoicesCompareAllFields)
{
    const NoteChoice choice{.value = 6, .label = "Achtel", .ticked = true};
    EXPECT_NE(choice, (NoteChoice{.value = 3, .label = "Achtel", .ticked = true}));
    EXPECT_NE(choice, (NoteChoice{.value = 6, .label = "Viertel", .ticked = true}));
    EXPECT_NE(choice, (NoteChoice{.value = 6, .label = "Achtel", .ticked = false}));
}

TEST_F(NoteCorrectionTest, FKO04_DeletesTheSelectedNote)
{
    notes.select(low);
    notes.deleteNote();
    EXPECT_EQ(take().noteEdits(), (std::vector<NoteEdit>{{.seconds = 1.5, .pitch = 35, .deleted = true}}));
    EXPECT_EQ(notes.selected(), std::nullopt);
}

TEST_F(NoteCorrectionTest, FKO04_TransposingAgainChangesTheSameEdit)
{
    notes.select(low);
    notes.transpose(2);
    notes.transpose(12);
    EXPECT_EQ(notes.selected()->pitch, 49);
    EXPECT_EQ(take().noteEdits(), (std::vector<NoteEdit>{{.seconds = 1.5, .pitch = 35, .toPitch = 49}}));
    notes.transpose(-14);
    EXPECT_EQ(take().noteEdits(), (std::vector<NoteEdit>{{.seconds = 1.5, .pitch = 35}}));
    notes.transpose(-100);
    EXPECT_EQ(notes.selected()->pitch, 0);
    notes.transpose(200);
    EXPECT_EQ(notes.selected()->pitch, 127);
}

TEST_F(NoteCorrectionTest, FKO04_TheDurationBelongsToTheEvent)
{
    notes.select(low);
    notes.setString(1);
    EXPECT_FALSE(notes.durationChoices()[3].ticked);
    notes.setString(kNone);
    notes.setDuration(6);
    EXPECT_EQ(take().noteEdits(),
              (std::vector<NoteEdit>{{.seconds = 1.5, .pitch = 35},
                                     {.seconds = 1.5, .pitch = kNone, .durationTicks = 6}}));
    const auto choices = notes.durationChoices();
    ASSERT_EQ(choices.size(), 11U);
    EXPECT_EQ(choices[3], (NoteChoice{.value = 6, .label = "Achtel", .ticked = true}));
    EXPECT_EQ(choices.back(), (NoteChoice{.value = 48, .label = "Ganze", .ticked = false}));
    notes.select(std::nullopt);
    EXPECT_FALSE(notes.durationChoices()[3].ticked);
}

TEST_F(NoteCorrectionTest, FTB03_TheNoteCanBePlayedOnTheStringsThatReachIt)
{
    // B1 on the five-string bass in G#: the low G# string at fret 15 up to the open B string; not the E
    // string.
    notes.select(low);
    auto choices = notes.stringChoices();
    ASSERT_EQ(choices.size(), 5U);
    EXPECT_EQ(choices[0], (NoteChoice{.value = kNone, .label = "Automatisch", .ticked = true}));
    EXPECT_EQ(choices[1], (NoteChoice{.value = 0, .label = "Saite 5, Bund 15", .ticked = false}));
    EXPECT_EQ(choices[3], (NoteChoice{.value = 2, .label = "Saite 3, Bund 5", .ticked = false}));
    EXPECT_EQ(choices[4], (NoteChoice{.value = 3, .label = "Saite 2, Bund 0", .ticked = false}));

    notes.setString(0);
    EXPECT_EQ(take().noteEdits(), (std::vector<NoteEdit>{{.seconds = 1.5, .pitch = 35, .string = 0}}));
    choices = notes.stringChoices();
    EXPECT_FALSE(choices[0].ticked);
    EXPECT_TRUE(choices[1].ticked);
    // A new pitch finds its string again.
    notes.transpose(1);
    EXPECT_EQ(take().noteEdits(), (std::vector<NoteEdit>{{.seconds = 1.5, .pitch = 35, .toPitch = 36}}));
    // B3 is out of reach on the three low strings, which end at fret 24.
    notes.transpose(23);
    choices = notes.stringChoices();
    ASSERT_EQ(choices.size(), 3U);
    EXPECT_EQ(choices[1].label, "Saite 2, Bund 24");
}

TEST_F(NoteCorrectionTest, FKO05_EditsAndTheirUndoQuantizeAgain)
{
    notes.select(low);
    notes.setString(1);
    const auto score = folder / kScoreFile;
    files.erase(score);
    EXPECT_TRUE(notes.poll());
    EXPECT_TRUE(files.contains(score));
    EXPECT_FALSE(notes.poll());

    undoManager.undo();
    EXPECT_TRUE(take().noteEdits().empty());
    files.erase(score);
    EXPECT_TRUE(notes.poll());
    EXPECT_TRUE(files.contains(score));
    undoManager.redo();
    EXPECT_TRUE(notes.poll());

    // Other changes of the project do not.
    project.setName("Anders");
    project.addVoice({.id = "v3", .name = "Neu", .profileId = "guitar6-e"});
    project.removeVoice(2);
    EXPECT_FALSE(notes.poll());
    EXPECT_TRUE(messages.empty());
}

TEST_F(NoteCorrectionTest, FKO05_ReportsWhatCannotBeWritten)
{
    notes.select(low);
    notes.deleteNote();
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(Return(false));
    EXPECT_TRUE(notes.poll());
    EXPECT_EQ(messages,
              (std::vector<std::string>{"Noten korrigieren: Der Rhythmus konnte nicht gespeichert "
                                        "werden; ist die Festplatte voll?"}));
}

TEST_F(NoteCorrectionTest, FKO05_NoUpdateWithoutAFolder)
{
    NoteCorrection withoutFolder{
        tree, undoManager, fileSystem, {.projectFolder = [] { return std::nullopt; }, .showMessage = {}}};
    withoutFolder.select(low);
    withoutFolder.deleteNote();
    EXPECT_FALSE(withoutFolder.poll());
}

} // namespace
} // namespace snotes::ui
