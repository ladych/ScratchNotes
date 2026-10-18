#include "ui/TranscriptionController.h"

#include "io/MockFileSystem.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace snotes::ui
{
namespace
{

namespace fs = std::filesystem;
using ::testing::_;
using ::testing::HasSubstr;
using ::testing::NiceMock;
using ::testing::Return;

/// Records the jobs; the test finishes them with finish().
class FakeTranscriptionWorker final : public ITranscriptionWorker
{
public:
    void start(TranscriptionJob job) override { jobs.push_back(std::move(job)); }
    void cancel() override { ++cancels; }
    [[nodiscard]] double progress() const override { return shownProgress; }
    [[nodiscard]] std::optional<TranscriptionOutcome> takeOutcome() override
    {
        return std::exchange(outcome, {});
    }

    void finish(TranscriptionOutcome::Status status, std::vector<transcription::Note> notes = {})
    {
        outcome = TranscriptionOutcome{.status = status, .notes = std::move(notes)};
    }

    std::vector<TranscriptionJob> jobs;
    int cancels = 0;
    double shownProgress = 0.0;
    std::optional<TranscriptionOutcome> outcome;
};

class TranscriptionControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto bass = project.addVoice({.id = "v1", .name = "Bass", .profileId = "bass5-gs"});
        bass.addTake({.id = "t1", .diFile = "audio/Bass Take 1 DI.wav"});
        bass.addTake({.id = "t2", .diFile = "audio/Bass Take 2 DI.wav"});
        auto guitar = project.addVoice({.id = "v2", .name = "Guitar", .profileId = "unknown"});
        guitar.addTake({.id = "t3", .diFile = "audio/Guitar Take 1 DI.wav"});
        project.addVoice({.id = "v3", .name = "Empty", .profileId = "guitar6-e"});
        ON_CALL(fileSystem, writeText(_, _)).WillByDefault(Return(true));
    }

    void finishWith(const std::vector<transcription::Note>& notes)
    {
        worker.finish(TranscriptionOutcome::Status::done, notes);
        controller.poll();
    }

    fs::path folder = fs::path{"projects"} / "Probe";
    std::optional<fs::path> currentFolder = folder;
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, nullptr};
    NiceMock<io::MockFileSystem> fileSystem;
    FakeTranscriptionWorker worker;
    std::vector<std::string> messages;
    TranscriptionController controller{
        tree,
        fileSystem,
        worker,
        {.projectFolder = [this] { return currentFolder; },
         .showMessage = [this](const std::string& /*title*/, const std::string& message)
         { messages.push_back(message); }}};
    const std::vector<transcription::Note> twoNotes{{.startSeconds = 0.5, .endSeconds = 1.0, .pitch = 30},
                                                    {.startSeconds = 1.0, .endSeconds = 2.0, .pitch = 32}};
};

TEST_F(TranscriptionControllerTest, FTX01_TranscribesTheActiveTakesOneAfterTheOther)
{
    controller.transcribeActiveTakes();

    ASSERT_EQ(worker.jobs.size(), 1U);
    EXPECT_EQ(worker.jobs[0].diFile, folder / "audio" / "Bass Take 2 DI.wav");
    EXPECT_DOUBLE_EQ(worker.jobs[0].sampleRate, project.sampleRate());
    EXPECT_TRUE(worker.jobs[0].settings.octaveUp);
    EXPECT_TRUE(controller.isRunning());

    controller.poll();
    EXPECT_EQ(worker.jobs.size(), 1U);

    EXPECT_CALL(fileSystem, writeText(_, _)).WillRepeatedly(Return(true));
    EXPECT_CALL(fileSystem,
                writeText(folder / "audio" / "Bass Take 2 DI.notes.csv", HasSubstr("0.5000,1.0000,30")))
        .WillOnce(Return(true));
    finishWith(twoNotes);

    const auto take = *project.voice(0).findTake("t2");
    EXPECT_EQ(take.notesFile(), "audio/Bass Take 2 DI.notes.csv");
    EXPECT_EQ(take.noteCount(), 2);
    ASSERT_EQ(worker.jobs.size(), 2U);
    // A voice with an unknown profile is transcribed as six-string guitar.
    EXPECT_FALSE(worker.jobs[1].settings.octaveUp);

    finishWith({});
    EXPECT_FALSE(controller.isRunning());
    EXPECT_EQ(project.voice(1).take(0).noteCount(), 0);
    EXPECT_EQ(project.voice(1).take(0).notesFile(), "audio/Guitar Take 1 DI.notes.csv");
}

TEST_F(TranscriptionControllerTest, FTX01_AfterRecordingOnlyTakesWithoutNotes)
{
    project.voice(0).take(0).setNotes("audio/Bass Take 1 DI.notes.csv", 5);

    controller.transcribeNew({"t1", "gone", "t3"});

    ASSERT_EQ(worker.jobs.size(), 1U);
    EXPECT_EQ(worker.jobs[0].diFile, folder / "audio" / "Guitar Take 1 DI.wav");
}

TEST_F(TranscriptionControllerTest, FTX01_TheCommandTranscribesAgainButQueuesEveryTakeOnce)
{
    project.voice(0).take(1).setNotes("audio/Bass Take 2 DI.notes.csv", 5);
    controller.transcribeActiveTakes();
    controller.transcribeActiveTakes();

    EXPECT_EQ(controller.statusText(), "Noten erkennen: Bass, Take 2, 0 % (1 weitere)");
    finishWith(twoNotes);
    finishWith(twoNotes);

    EXPECT_EQ(worker.jobs.size(), 2U);
    EXPECT_FALSE(controller.isRunning());
    EXPECT_EQ(project.voice(0).take(1).noteCount(), 2);
}

TEST_F(TranscriptionControllerTest, FTX01_ShowsTheProgress)
{
    controller.poll();
    EXPECT_EQ(controller.statusText(), "");

    controller.transcribeNew({"t3"});
    worker.shownProgress = 0.404;

    EXPECT_EQ(controller.statusText(), "Noten erkennen: Guitar, Take 1, 40 %");

    project.removeVoice(1);
    EXPECT_EQ(controller.statusText(), "Noten erkennen: 40 %");
}

TEST_F(TranscriptionControllerTest, FTX01_CancelStopsTheTakeAndForgetsTheQueue)
{
    controller.cancel();
    EXPECT_EQ(worker.cancels, 0);

    controller.transcribeActiveTakes();
    controller.cancel();
    worker.finish(TranscriptionOutcome::Status::cancelled);
    controller.poll();

    EXPECT_EQ(worker.cancels, 1);
    EXPECT_EQ(worker.jobs.size(), 1U);
    EXPECT_FALSE(controller.isRunning());
    EXPECT_EQ(project.voice(0).take(1).notesFile(), "");
    EXPECT_TRUE(messages.empty());
}

TEST_F(TranscriptionControllerTest, FTX01_FailuresAreReportedAndTheQueueGoesOn)
{
    controller.transcribeActiveTakes();
    worker.finish(TranscriptionOutcome::Status::failed);
    controller.poll();

    ASSERT_EQ(messages.size(), 1U);
    EXPECT_THAT(messages[0], HasSubstr("DI-Datei"));
    EXPECT_EQ(worker.jobs.size(), 2U);

    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(Return(false));
    finishWith(twoNotes);

    ASSERT_EQ(messages.size(), 2U);
    EXPECT_THAT(messages[1], HasSubstr("Festplatte"));
    EXPECT_EQ(project.voice(1).take(0).notesFile(), "");
}

TEST_F(TranscriptionControllerTest, FTX01_SkipsTakesWithoutAudioOrProject)
{
    auto missing = project.voice(0).take(1).tree();
    missing.setProperty(model::ids::audioMissing, true, nullptr);
    controller.transcribeActiveTakes();
    ASSERT_EQ(worker.jobs.size(), 1U);
    EXPECT_EQ(worker.jobs[0].diFile, folder / "audio" / "Guitar Take 1 DI.wav");

    currentFolder.reset();
    EXPECT_CALL(fileSystem, writeText(_, _)).Times(0);
    finishWith(twoNotes);
    controller.transcribeActiveTakes();

    EXPECT_EQ(worker.jobs.size(), 1U);
    EXPECT_EQ(project.voice(1).take(0).notesFile(), "");
}

TEST_F(TranscriptionControllerTest, FTX01_ADeletedTakeIsNotStored)
{
    controller.transcribeNew({"t3"});
    project.removeVoice(1);

    EXPECT_CALL(fileSystem, writeText(_, _)).Times(0);
    finishWith(twoNotes);

    EXPECT_FALSE(controller.isRunning());
}

TEST_F(TranscriptionControllerTest, FTX01_AQueuedTakeDeletedMeanwhileIsSkipped)
{
    controller.transcribeActiveTakes();
    project.removeVoice(1);

    finishWith(twoNotes);

    EXPECT_EQ(worker.jobs.size(), 1U);
    EXPECT_FALSE(controller.isRunning());
}

TEST(NotesFileTest, FTX01_LiesNextToTheDiFile)
{
    EXPECT_EQ(notesFileFor("audio/Bass Take 1 DI.wav"), "audio/Bass Take 1 DI.notes.csv");
}

} // namespace
} // namespace snotes::ui
