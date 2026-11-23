#include "ui/TakeImport.h"

#include "io/FakeBinaryFile.h"
#include "io/MockFileSystem.h"
#include "io/WavWriter.h"
#include "model/FakeIdGenerator.h"
#include "model/ModelIds.h"
#include "model/ProjectFactory.h"
#include "ui/FakeDocumentView.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
using ::testing::FloatEq;
using ::testing::HasSubstr;
using ::testing::Invoke;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;

/// Offset of the first sample and of the number of samples in a file of io::WavWriter.
constexpr std::size_t kFirstSample = io::WavWriter::kHeaderSize;
constexpr std::size_t kSampleCount = 46;

class FakeAudioFileReader final : public io::IAudioFileReader
{
public:
    [[nodiscard]] std::optional<io::AudioFileContent> read(const fs::path& file) override
    {
        const auto found = contents.find(file.generic_string());
        if (found == contents.end())
            return std::nullopt;
        return found->second;
    }

    std::map<std::string, io::AudioFileContent> contents;
};

TEST(AssignImportFilesTest, FPJ05_TheFileNamedDiIsTheDiSignal)
{
    const auto files = assignImportFiles("Riff bearbeitet.wav", "Riff DI.wav");

    EXPECT_EQ(files.di, fs::path{"Riff DI.wav"});
    EXPECT_EQ(files.processed, fs::path{"Riff bearbeitet.wav"});
}

TEST(AssignImportFilesTest, FPJ05_DryMarksTheDiSignalInAnyCase)
{
    const auto files = assignImportFiles("riff_DRY.wav", "riff_amp.wav");

    EXPECT_EQ(files.di, fs::path{"riff_DRY.wav"});
    EXPECT_EQ(files.processed, fs::path{"riff_amp.wav"});
}

TEST(AssignImportFilesTest, FPJ05_WithoutDiTheOtherFileOfAProcessedOneIsTheDiSignal)
{
    const auto files = assignImportFiles("Take 3 wet.wav", "Take 3.wav");

    EXPECT_EQ(files.di, fs::path{"Take 3.wav"});
    EXPECT_EQ(files.processed, fs::path{"Take 3 wet.wav"});
}

TEST(AssignImportFilesTest, FPJ05_WithoutAWordThatTellsTheFirstFileIsTheDiSignal)
{
    const auto files = assignImportFiles("a.wav", "b.wav");

    EXPECT_EQ(files.di, fs::path{"a.wav"});
    EXPECT_EQ(files.processed, fs::path{"b.wav"});
}

TEST(AssignImportFilesTest, FPJ05_TwoProcessedFilesKeepTheirOrder)
{
    const auto files = assignImportFiles("a fx.wav", "b-processed.wav");

    EXPECT_EQ(files.di, fs::path{"a fx.wav"});
}

TEST(VoiceNameFromFileTest, FPJ05_LeavesOutTheExtensionAndTheSignalWords)
{
    EXPECT_EQ(voiceNameFromFile("Stimme 1 Take 2 DI.wav"), "Stimme 1 Take 2");
    EXPECT_EQ(voiceNameFromFile("riff_dry-take..wav"), "riff take");
    EXPECT_EQ(voiceNameFromFile("Solo  2 - bearbeitet.wav"), "Solo 2");
}

TEST(VoiceNameFromFileTest, FPJ05_AFileNamedOnlyDiKeepsItsName)
{
    EXPECT_EQ(voiceNameFromFile("DI.wav"), "DI");
}

TEST(MonoAtRateTest, FPJ05_MixesTheChannelsToTheirMean)
{
    const io::AudioFileContent content{.sampleRate = 48000.0, .channels = {{1.0F, 0.5F}, {0.0F, -0.5F}}};

    EXPECT_THAT(monoAtRate(content, 48000.0), ElementsAre(FloatEq(0.5F), FloatEq(0.0F)));
}

TEST(MonoAtRateTest, FPJ05_ConvertsToTheSampleRate)
{
    const io::AudioFileContent content{.sampleRate = 96000.0, .channels = {std::vector<float>(960, 0.25F)}};

    EXPECT_EQ(monoAtRate(content, 48000.0).size(), 480U);
}

TEST(MonoAtRateTest, FPJ05_NoChannelsGiveNoSamples)
{
    EXPECT_THAT(monoAtRate(io::AudioFileContent{.sampleRate = 48000.0, .channels = {}}, 48000.0), IsEmpty());
}

class TakeImportTest : public ::testing::Test
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
                    content->failAtCall = failAtCall;
                    files[file.generic_string()] = content;
                    return std::make_unique<FakeBinaryFile>(content);
                }));
        guitar = project.addVoice(
            {.id = "g", .name = "Gitarre 1", .profileId = "guitar6-e", .inputDi = 0, .inputWet = 1});
        undoManager.clearUndoHistory();
        reader.contents["/home/alex/Reaper/Riff DI.wav"] = {.sampleRate = 44100.0,
                                                            .channels = {{0.5F, -0.5F}}};
        reader.contents["/home/alex/Reaper/Riff bearbeitet.wav"] = {
            .sampleRate = 44100.0, .channels = {{0.25F, 0.75F}, {0.75F, 0.25F}}};
    }

    void choose(std::vector<fs::path> chosen) { view.audioFilesToImport = std::move(chosen); }

    [[nodiscard]] const FakeFileContent& written(const std::string& file) const
    {
        return *files.at((folder / file).generic_string());
    }

    fs::path folder = fs::path{"Band"} / "Probe";
    std::optional<fs::path> projectFolder = folder;
    bool recording = false;
    int failAtCall = 0;
    std::vector<std::string> messages;
    std::vector<std::string> imported;
    std::map<std::string, std::shared_ptr<FakeFileContent>> files;

    juce::UndoManager undoManager;
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, &undoManager};
    model::Voice guitar{{}, nullptr};
    NiceMock<io::MockFileSystem> fileSystem;
    FakeAudioFileReader reader;
    model::FakeIdGenerator idGenerator;
    FakeDocumentView view;
    TakeImport import{tree,
                      undoManager,
                      fileSystem,
                      reader,
                      idGenerator,
                      view,
                      {.projectFolder = [this](const TakeImportEnvironment::FolderCallback& onFolder)
                       { onFolder(projectFolder); },
                       .isRecording = [this] { return recording; },
                       .numInputs = [] { return 4; },
                       .now = [] { return std::string{"2026-11-23T20:15:00"}; },
                       .showMessage = [this](const std::string& title, const std::string& message)
                       { messages.push_back(title + ": " + message); },
                       .takesImported = [this](const std::vector<std::string>& ids)
                       { imported.insert(imported.end(), ids.begin(), ids.end()); }}};
};

TEST_F(TakeImportTest, FPJ05_ImportsAFileAsTheActiveTakeOfTheVoice)
{
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");

    ASSERT_EQ(guitar.numTakes(), 1);
    const auto take = guitar.take(0);
    EXPECT_EQ(take.diFile(), "audio/Gitarre 1 Take 1 DI.wav");
    EXPECT_EQ(take.wetFile(), "");
    EXPECT_EQ(take.startSample(), 0);
    EXPECT_EQ(take.latencyOffsetSamples(), 0);
    EXPECT_EQ(take.recordedAt(), "2026-11-23T20:15:00");
    EXPECT_EQ(guitar.activeTake()->id(), take.id());
    EXPECT_THAT(imported, ElementsAre(take.id()));
    EXPECT_THAT(messages, IsEmpty());
}

TEST_F(TakeImportTest, FPJ05_CopiesTheSamplesAsMonoWavFile)
{
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");

    const auto& file = written("audio/Gitarre 1 Take 1 DI.wav");
    EXPECT_EQ(file.uint32At(kSampleCount), 2U);
    EXPECT_FLOAT_EQ(file.floatAt(kFirstSample), 0.5F);
    EXPECT_FLOAT_EQ(file.floatAt(kFirstSample + 4), -0.5F);
    EXPECT_GE(file.flushes, 1);
}

TEST_F(TakeImportTest, FPJ05_AProjectWithoutTakesTakesTheSampleRateOfTheFile)
{
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");

    EXPECT_DOUBLE_EQ(project.sampleRate(), 44100.0);
    EXPECT_EQ(written("audio/Gitarre 1 Take 1 DI.wav").uint32At(24), 44100U);
}

TEST_F(TakeImportTest, FPJ05_ConvertsAFileToTheSampleRateOfTheTakes)
{
    tree.setProperty(model::ids::sampleRate, 88200.0, nullptr);
    static_cast<void>(guitar.addTake({.id = "t1", .diFile = "audio/Gitarre 1 Take 1 DI.wav"}));
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");

    const auto& file = written("audio/Gitarre 1 Take 2 DI.wav");
    EXPECT_EQ(file.uint32At(24), 88200U);
    EXPECT_EQ(file.uint32At(kSampleCount), 4U);
    EXPECT_DOUBLE_EQ(project.sampleRate(), 88200.0);
}

TEST_F(TakeImportTest, FPJ05_ImportsTheDiAndTheProcessedSignal)
{
    choose({"/home/alex/Reaper/Riff bearbeitet.wav", "/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");

    ASSERT_EQ(guitar.numTakes(), 1);
    EXPECT_EQ(guitar.take(0).diFile(), "audio/Gitarre 1 Take 1 DI.wav");
    EXPECT_EQ(guitar.take(0).wetFile(), "audio/Gitarre 1 Take 1 bearbeitet.wav");
    const auto& processed = written("audio/Gitarre 1 Take 1 bearbeitet.wav");
    EXPECT_FLOAT_EQ(processed.floatAt(kFirstSample), 0.5F);
    EXPECT_FLOAT_EQ(processed.floatAt(kFirstSample + 4), 0.5F);
}

TEST_F(TakeImportTest, FPJ05_TheTakeNumberSkipsFilesThatExist)
{
    ON_CALL(fileSystem, exists(folder / "audio" / "Gitarre 1 Take 1 DI.wav")).WillByDefault(Return(true));
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");

    EXPECT_EQ(guitar.take(0).diFile(), "audio/Gitarre 1 Take 2 DI.wav");
}

TEST_F(TakeImportTest, FPJ04_TheImportIsOneUndoStep)
{
    static_cast<void>(guitar.addTake({.id = "t1", .diFile = "audio/Gitarre 1 Take 1 DI.wav"}));
    choose({"/home/alex/Reaper/Riff bearbeitet.wav", "/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");
    ASSERT_EQ(guitar.numTakes(), 2);
    undoManager.undo();

    EXPECT_EQ(guitar.numTakes(), 1);
}

TEST_F(TakeImportTest, FPJ05_ImportsAsANewVoiceNamedAfterTheFile)
{
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importAsNewVoice();

    ASSERT_EQ(project.numVoices(), 2);
    const auto voice = project.voice(1);
    EXPECT_EQ(voice.name(), "Riff");
    ASSERT_EQ(voice.numTakes(), 1);
    EXPECT_EQ(voice.take(0).diFile(), "audio/Riff Take 1 DI.wav");
    EXPECT_THAT(imported, ElementsAre(voice.take(0).id()));
}

TEST_F(TakeImportTest, FPJ04_TheNewVoiceAndItsTakeAreOneUndoStep)
{
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importAsNewVoice();
    undoManager.undo();

    EXPECT_EQ(project.numVoices(), 1);
}

TEST_F(TakeImportTest, FPJ05_TheNextDialogStartsInTheFolderOfTheLastFile)
{
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");
    import.importInto("g");

    EXPECT_THAT(view.audioStartFolders, ElementsAre(fs::path{}, fs::path{"/home/alex/Reaper"}));
}

TEST_F(TakeImportTest, FPJ05_CancellingTheDialogImportsNothing)
{
    choose({});

    import.importInto("g");

    EXPECT_EQ(guitar.numTakes(), 0);
    EXPECT_THAT(messages, IsEmpty());
}

TEST_F(TakeImportTest, FPJ05_MoreThanTwoFilesAreRefused)
{
    choose({"/a/1.wav", "/a/2.wav", "/a/3.wav"});

    import.importInto("g");

    EXPECT_EQ(guitar.numTakes(), 0);
    EXPECT_THAT(messages, ElementsAre(HasSubstr("eine Datei wählen oder zwei")));
}

TEST_F(TakeImportTest, FPJ05_NothingIsImportedWhileRecording)
{
    recording = true;
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importAsNewVoice();

    EXPECT_THAT(view.audioStartFolders, IsEmpty());
    EXPECT_THAT(messages, ElementsAre(HasSubstr("zuerst die Aufnahme beenden")));
}

TEST_F(TakeImportTest, FPJ05_AFileThatIsNoAudioIsReported)
{
    choose({"/home/alex/Reaper/Riff DI.wav", "/home/alex/Reaper/notes.txt"});

    import.importInto("g");

    EXPECT_EQ(guitar.numTakes(), 0);
    EXPECT_THAT(messages, ElementsAre("Import: „notes.txt“ konnte nicht als Audio-Datei gelesen werden."));
}

TEST_F(TakeImportTest, FPJ05_NewVoiceFromAFileThatIsNoAudioIsNotAdded)
{
    choose({"/home/alex/Reaper/notes.txt"});

    import.importAsNewVoice();

    EXPECT_EQ(project.numVoices(), 1);
}

TEST_F(TakeImportTest, FPJ05_NothingIsImportedWithoutProjectFolder)
{
    projectFolder.reset();
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");

    EXPECT_EQ(guitar.numTakes(), 0);
    EXPECT_THAT(files, IsEmpty());
}

TEST_F(TakeImportTest, FPJ05_AVoiceRemovedMeanwhileGetsNothing)
{
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("removed");

    EXPECT_THAT(files, IsEmpty());
    EXPECT_THAT(imported, IsEmpty());
}

TEST_F(TakeImportTest, FPJ05_ReportsAnAudioFolderThatCannotBeCreated)
{
    ON_CALL(fileSystem, createDirectories(_)).WillByDefault(Return(false));
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");

    EXPECT_EQ(guitar.numTakes(), 0);
    EXPECT_THAT(messages, ElementsAre(HasSubstr("nicht angelegt werden")));
}

TEST_F(TakeImportTest, FPJ05_ReportsAFileThatCannotBeCreated)
{
    ON_CALL(fileSystem, createBinaryFile(_))
        .WillByDefault(Invoke([](const fs::path&) { return std::unique_ptr<io::IBinaryFile>{}; }));
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importInto("g");

    EXPECT_EQ(guitar.numTakes(), 0);
    EXPECT_THAT(messages,
                ElementsAre("Import: „audio/Gitarre 1 Take 1 DI.wav“ konnte nicht angelegt werden."));
}

TEST_F(TakeImportTest, FPJ05_ReportsAFailedWriteAndAddsNoVoice)
{
    failAtCall = 2; // the samples after the header
    choose({"/home/alex/Reaper/Riff DI.wav"});

    import.importAsNewVoice();

    EXPECT_EQ(project.numVoices(), 1);
    EXPECT_THAT(messages, ElementsAre(HasSubstr("Ist die Festplatte voll?")));
    EXPECT_THAT(imported, IsEmpty());
}

} // namespace
} // namespace snotes::ui
