#include "ui/RecordingController.h"

#include "io/FakeBinaryFile.h"
#include "io/MockFileSystem.h"
#include "model/FakeIdGenerator.h"
#include "model/ProjectFactory.h"
#include "ui/FakeWriterThread.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace snotes::ui
{
namespace
{

namespace fs = std::filesystem;
using io::FakeBinaryFile;
using io::FakeFileContent;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using State = RecordingController::State;

/// A recorder whose audio thread can be held inside a block.
class FakeRecorder final : public engine::IRecorderControl
{
public:
    void start(engine::RecordingSession& session) noexcept override { real.start(session); }
    void stop() noexcept override { real.stop(); }
    [[nodiscard]] bool isReleased(const engine::RecordingSession& session) const noexcept override
    {
        return !busy && real.isReleased(session);
    }

    engine::Recorder real;
    bool busy = false;
};

class RecordingControllerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(true));
        ON_CALL(fileSystem, exists(_)).WillByDefault(Return(false));
        ON_CALL(fileSystem, createBinaryFile(_))
            .WillByDefault(Invoke(
                [this](const fs::path& file) -> std::unique_ptr<io::IBinaryFile>
                {
                    auto content = std::make_shared<FakeFileContent>();
                    files[file.generic_string()] = content;
                    return std::make_unique<FakeBinaryFile>(content);
                }));
        guitar = project.addVoice(
            {.id = "g", .name = "Gitarre 1", .profileId = "guitar6-e", .inputDi = 0, .inputWet = 1});
        bass = project.addVoice({.id = "b", .name = "Bass", .profileId = "bass4-e", .inputDi = 2});
        undoManager.clearUndoHistory();
    }

    /// The audio thread processes one block of `numSamples` with input n carrying the value n + 1.
    void audioBlock(int numSamples)
    {
        const auto block = transport.beginBlock();
        std::array<std::vector<float>, 3> inputs;
        std::array<const float*, 3> channels{};
        for (std::size_t input = 0; input < inputs.size(); ++input)
        {
            inputs.at(input).assign(static_cast<std::size_t>(numSamples), static_cast<float>(input + 1));
            channels.at(input) = inputs.at(input).data();
        }
        if (block.playing)
            recorder.real.process(channels.data(), 3, numSamples, block.position);
        transport.endBlock(block, numSamples);
    }

    void recordPass(int numSamples)
    {
        controller.start();
        audioBlock(numSamples);
        controller.stop();
        audioBlock(numSamples);
        controller.poll();
    }

    [[nodiscard]] std::uint32_t samplesIn(const std::string& file) const
    {
        return files.at((folder / file).generic_string())->uint32At(46);
    }

    fs::path folder = fs::path{"Band"} / "Probe";
    std::optional<fs::path> projectFolder = folder;
    double sampleRate = 48000.0;
    std::vector<std::string> messages;
    std::vector<std::string> recordedTakes;
    std::map<std::string, std::shared_ptr<FakeFileContent>> files;

    juce::UndoManager undoManager;
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, &undoManager};
    model::Voice guitar{{}, nullptr};
    model::Voice bass{{}, nullptr};
    FakeRecorder recorder;
    engine::Transport transport;
    std::int64_t latency = 0;
    NiceMock<io::MockFileSystem> fileSystem;
    FakeWriterThread writerThread;
    model::FakeIdGenerator idGenerator;
    RecordingController controller{
        tree,
        undoManager,
        recorder,
        transport,
        fileSystem,
        writerThread,
        idGenerator,
        {.projectFolder = [this](const RecordingEnvironment::FolderCallback& onFolder)
         { onFolder(projectFolder); },
         .deviceSampleRate = [this] { return sampleRate; },
         .latencySamples = [this] { return latency; },
         .now = [] { return std::string{"2026-10-17T09:40:00"}; },
         .showMessage = [this](const std::string& /*title*/, const std::string& message)
         { messages.push_back(message); },
         .takesRecorded = [this](const std::vector<std::string>& takeIds) { recordedTakes = takeIds; }}};
};

TEST_F(RecordingControllerTest, FAU05_ArmsVoices)
{
    controller.setArmed("g", true);
    controller.setArmed("b", true);
    controller.setArmed("b", false);

    EXPECT_TRUE(controller.isArmed("g"));
    EXPECT_FALSE(controller.isArmed("b"));
}

TEST_F(RecordingControllerTest, FAU02_NothingArmedRecordsNothing)
{
    controller.start();

    EXPECT_EQ(controller.state(), State::idle);
    ASSERT_EQ(messages.size(), 1U);
    EXPECT_THAT(messages[0], HasSubstr("scharf"));
}

TEST_F(RecordingControllerTest, FAU02_VoiceWithoutDiInputCannotRecord)
{
    bass.setInputDi(model::kNoInput);
    controller.setArmed("b", true);

    controller.start();

    EXPECT_EQ(controller.state(), State::idle);
}

TEST_F(RecordingControllerTest, FAU02_NoDeviceRecordsNothing)
{
    controller.setArmed("g", true);
    sampleRate = 0.0;

    controller.start();

    EXPECT_EQ(controller.state(), State::idle);
    EXPECT_THAT(messages.at(0), HasSubstr("Kein Audiogerät"));
}

TEST_F(RecordingControllerTest, FAU02_ProjectWithoutFolderRecordsNothing)
{
    controller.setArmed("g", true);
    projectFolder.reset();

    controller.start();

    EXPECT_EQ(controller.state(), State::idle);
}

TEST_F(RecordingControllerTest, FAU02_RecordsDiAndProcessedSignalIntoTheirFiles)
{
    controller.setArmed("g", true);

    controller.start();
    EXPECT_EQ(controller.state(), State::recording);
    EXPECT_NE(writerThread.attached, nullptr);
    audioBlock(100);
    EXPECT_DOUBLE_EQ(controller.recordedSeconds(), 100.0 / 48000.0);
    controller.stop();
    controller.poll();

    EXPECT_EQ(controller.state(), State::idle);
    EXPECT_EQ(writerThread.detaches, 1);
    EXPECT_EQ(samplesIn("audio/Gitarre 1 Take 1 DI.wav"), 100U);
    EXPECT_EQ(samplesIn("audio/Gitarre 1 Take 1 bearbeitet.wav"), 100U);
    EXPECT_FLOAT_EQ(files.at((folder / "audio/Gitarre 1 Take 1 bearbeitet.wav").generic_string())
                        ->floatAt(io::WavWriter::kHeaderSize),
                    2.0F);
    EXPECT_DOUBLE_EQ(controller.recordedSeconds(), 0.0);
}

TEST_F(RecordingControllerTest, FWI04_RecordingRunsWithTheTransport)
{
    controller.setArmed("g", true);

    controller.start();
    EXPECT_TRUE(transport.isPlaying());
    controller.stop();

    EXPECT_FALSE(transport.isPlaying());
}

TEST_F(RecordingControllerTest, FWI05_TakeRemembersItsSongPositionAndTheLatency)
{
    controller.setArmed("g", true);
    transport.locate(4800);
    latency = 240;

    recordPass(100);

    const auto take = guitar.activeTake();
    ASSERT_TRUE(take.has_value());
    EXPECT_EQ(take->startSample(), 4800);
    EXPECT_EQ(take->latencyOffsetSamples(), 240);
}

TEST_F(RecordingControllerTest, FWI05_TakeWithoutAudioStartsAtTheSongStart)
{
    controller.setArmed("g", true);

    controller.start();
    controller.stop();
    controller.poll();

    ASSERT_TRUE(guitar.activeTake().has_value());
    EXPECT_EQ(guitar.activeTake()->startSample(), 0);
}

TEST_F(RecordingControllerTest, FAU04_EachPassBecomesTheActiveTake)
{
    controller.setArmed("g", true);

    recordPass(10);

    ASSERT_EQ(guitar.numTakes(), 1);
    const auto take = guitar.activeTake();
    EXPECT_EQ(take->id(), "id-1");
    EXPECT_EQ(take->diFile(), "audio/Gitarre 1 Take 1 DI.wav");
    EXPECT_EQ(take->wetFile(), "audio/Gitarre 1 Take 1 bearbeitet.wav");
    EXPECT_EQ(take->recordedAt(), "2026-10-17T09:40:00");
    EXPECT_EQ(take->startSample(), 0);
    EXPECT_TRUE(messages.empty());
}

TEST_F(RecordingControllerTest, FAU05_ArmedVoicesRecordTogether)
{
    controller.setArmed("g", true);
    controller.setArmed("b", true);

    recordPass(10);

    EXPECT_EQ(guitar.numTakes(), 1);
    EXPECT_EQ(bass.numTakes(), 1);
    EXPECT_EQ(bass.activeTake()->wetFile(), "");
    EXPECT_EQ(samplesIn("audio/Bass Take 1 DI.wav"), 10U);
    EXPECT_THAT(recordedTakes, ElementsAre(guitar.activeTake()->id(), bass.activeTake()->id()));
}

TEST_F(RecordingControllerTest, FPJ04_APassIsOneUndoStep)
{
    controller.setArmed("g", true);
    controller.setArmed("b", true);
    recordPass(10);

    undoManager.undo();

    EXPECT_EQ(guitar.numTakes(), 0);
    EXPECT_EQ(bass.numTakes(), 0);
}

TEST_F(RecordingControllerTest, FAU04_FileNamesSkipExistingFiles)
{
    controller.setArmed("b", true);
    EXPECT_CALL(fileSystem, exists(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystem, exists(folder / "audio" / "Bass Take 1 DI.wav")).WillOnce(Return(true));

    recordPass(10);

    EXPECT_EQ(bass.activeTake()->diFile(), "audio/Bass Take 2 DI.wav");
}

TEST_F(RecordingControllerTest, FAU04_FileNamesSkipExistingProcessedFiles)
{
    controller.setArmed("b", true);
    EXPECT_CALL(fileSystem, exists(_)).WillRepeatedly(Return(false));
    EXPECT_CALL(fileSystem, exists(folder / "audio" / "Bass Take 1 bearbeitet.wav")).WillOnce(Return(true));

    recordPass(10);

    EXPECT_EQ(bass.activeTake()->diFile(), "audio/Bass Take 2 DI.wav");
}

TEST_F(RecordingControllerTest, FAU02_FileNamesAreLegal)
{
    guitar.setName("AC/DC: Riff?");
    controller.setArmed("g", true);

    recordPass(10);

    EXPECT_EQ(guitar.activeTake()->diFile(), "audio/ACDC Riff Take 1 DI.wav");
}

TEST_F(RecordingControllerTest, FAU02_SetsTheSampleRateOfTheFirstTakes)
{
    sampleRate = 44100.0;
    controller.setArmed("b", true);

    recordPass(10);

    EXPECT_DOUBLE_EQ(project.sampleRate(), 44100.0);
    EXPECT_EQ(files.begin()->second->uint32At(24), 44100U);
}

TEST_F(RecordingControllerTest, FAU02_RefusesADifferentSampleRateOnceThereAreTakes)
{
    controller.setArmed("b", true);
    recordPass(10);
    sampleRate = 44100.0;

    controller.start();

    EXPECT_EQ(controller.state(), State::idle);
    EXPECT_THAT(messages.at(0), HasSubstr("44100 Hz"));
}

TEST_F(RecordingControllerTest, FAU02_SameSampleRateRecordsMoreTakes)
{
    controller.setArmed("b", true);
    recordPass(10);

    recordPass(10);

    EXPECT_EQ(bass.numTakes(), 2);
    EXPECT_EQ(bass.activeTake()->diFile(), "audio/Bass Take 2 DI.wav");
}

TEST_F(RecordingControllerTest, FAU02_FailedFolderStopsTheStart)
{
    controller.setArmed("g", true);
    EXPECT_CALL(fileSystem, createDirectories(_)).WillOnce(Return(false));

    controller.start();

    EXPECT_EQ(controller.state(), State::idle);
    EXPECT_THAT(messages.at(0), HasSubstr("Band"));
}

TEST_F(RecordingControllerTest, FAU02_FailedFileStopsTheStart)
{
    controller.setArmed("g", true);
    EXPECT_CALL(fileSystem, createBinaryFile(_)).WillOnce(Return(nullptr));

    controller.start();

    EXPECT_EQ(controller.state(), State::idle);
    EXPECT_THAT(messages.at(0), HasSubstr("Gitarre 1 Take 1 DI.wav"));
}

TEST_F(RecordingControllerTest, FAU02_WaitsForTheAudioThreadBeforeFinishing)
{
    controller.setArmed("b", true);
    controller.start();
    audioBlock(10);

    // The audio thread is inside a block while the GUI thread stops.
    recorder.busy = true;
    controller.stop();
    EXPECT_EQ(controller.state(), State::stopping);
    controller.poll();
    EXPECT_EQ(bass.numTakes(), 0);

    recorder.busy = false;
    controller.poll();
    EXPECT_EQ(controller.state(), State::idle);
    EXPECT_EQ(bass.numTakes(), 1);
}

TEST_F(RecordingControllerTest, FAU02_ToggleStartsAndStops)
{
    controller.setArmed("b", true);

    controller.toggle();
    EXPECT_EQ(controller.state(), State::recording);
    controller.toggle();

    EXPECT_EQ(controller.state(), State::idle);
    EXPECT_EQ(bass.numTakes(), 1);
}

TEST_F(RecordingControllerTest, FAU02_StartWhileRecordingAndStopWhileIdleDoNothing)
{
    controller.setArmed("b", true);
    controller.stop();
    controller.start();

    controller.start();
    controller.poll();

    EXPECT_EQ(controller.state(), State::recording);
    EXPECT_EQ(idGenerator.next(), "id-2");
}

TEST_F(RecordingControllerTest, FAU04_RemovedVoiceGetsNoTake)
{
    controller.setArmed("g", true);
    controller.setArmed("b", true);
    controller.start();
    project.removeVoice(1);

    controller.stop();

    EXPECT_EQ(guitar.numTakes(), 1);
    EXPECT_THAT(recordedTakes, ElementsAre(guitar.activeTake()->id()));
}

TEST_F(RecordingControllerTest, FAU02_WriteErrorsAreReported)
{
    controller.setArmed("b", true);
    controller.start();
    files.begin()->second->failAtCall = 2;
    audioBlock(10);

    controller.stop();

    ASSERT_EQ(messages.size(), 1U);
    EXPECT_THAT(messages[0], HasSubstr("Festplatte"));
    EXPECT_EQ(bass.numTakes(), 1);
}

TEST_F(RecordingControllerTest, FAU02_DroppedSamplesAreReported)
{
    controller.setArmed("b", true);
    controller.start();
    audioBlock(static_cast<int>(engine::RecordingSession::kDefaultRingCapacity) + 5);

    controller.stop();

    ASSERT_EQ(messages.size(), 1U);
    EXPECT_THAT(messages[0], HasSubstr("5 Samples"));
}

TEST_F(RecordingControllerTest, FAU02_ClosingWhileRecordingKeepsTheAudio)
{
    auto content = std::make_shared<FakeFileContent>();
    {
        RecordingController closing{
            tree,
            undoManager,
            recorder,
            transport,
            fileSystem,
            writerThread,
            idGenerator,
            {.projectFolder = [this](const RecordingEnvironment::FolderCallback& onFolder)
             { onFolder(projectFolder); },
             .deviceSampleRate = [] { return 48000.0; },
             .latencySamples = [] { return std::int64_t{0}; },
             .now = [] { return std::string{}; },
             .showMessage = [](const std::string&, const std::string&) {}}};
        closing.setArmed("b", true);
        closing.start();
        audioBlock(10);
    }

    EXPECT_EQ(samplesIn("audio/Bass Take 1 DI.wav"), 10U);
    EXPECT_EQ(writerThread.detaches, 1);
}

} // namespace
} // namespace snotes::ui
