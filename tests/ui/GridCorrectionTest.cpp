#include "ui/GridCorrection.h"

#include "i18n/Translate.h"
#include "io/MockFileSystem.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "rhythm/RhythmCsv.h"
#include "rhythm/RhythmReference.h"
#include "transcription/NoteCsv.h"

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
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::NiceMock;
using ::testing::Return;

/// A grid of beats every half second at 48 kHz in 4/4 from the second beat, 3/4 as second reading.
class GridCorrectionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        project.beatGrid().setAnalysis({0, 24000, 48000, 72000, 96000},
                                       {{.numerator = 4, .confidence = 0.674, .phase = 1},
                                        {.numerator = 3, .confidence = 0.2, .phase = 2}});
    }

    [[nodiscard]] std::vector<std::int64_t> beats() const { return project.beatGrid().beats(); }

    std::optional<fs::path> folder;
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, nullptr};
    NiceMock<io::MockFileSystem> fileSystem;
    std::vector<std::string> messages;
    GridCorrection correction{tree,
                              fileSystem,
                              {.projectFolder = [this] { return folder; },
                               .showMessage = [this](const std::string& /*title*/, const std::string& text)
                               { messages.push_back(text); }}};
};

TEST_F(GridCorrectionTest, FKO01_TheReadingsOfTheAnalysisAreTheChoice)
{
    EXPECT_THAT(correction.meterChoices(),
                ElementsAre(MeterChoice{.label = "4/4 (67 %)", .ticked = true},
                            MeterChoice{.label = "3/4 (20 %)", .ticked = false}));
    EXPECT_NE(MeterChoice{.label = "4/4"}, MeterChoice{.label = "3/4"});
    EXPECT_NE((MeterChoice{.label = "4/4", .ticked = true}), (MeterChoice{.label = "4/4", .ticked = false}));
}

TEST_F(GridCorrectionTest, FKO01_ChoosingAReadingTakesItsMeterAndDownbeat)
{
    correction.chooseMeter(1);

    const auto grid = project.beatGrid();
    EXPECT_TRUE(grid.isEdited());
    EXPECT_EQ(grid.chosenMeter(), 3);
    EXPECT_EQ(grid.downbeatPhase(), 2);
    EXPECT_EQ(beats().size(), 5U);
    EXPECT_TRUE(correction.meterChoices()[1].ticked);

    correction.chooseMeter(2);
    EXPECT_EQ(project.beatGrid().chosenMeter(), 3);
}

TEST_F(GridCorrectionTest, FKO02_FindsTheBeatUnderTheMouse)
{
    EXPECT_EQ(correction.beatNear(0.52, 0.05), 1U);
    EXPECT_EQ(correction.beatNear(0.97, 0.05), 2U);
    EXPECT_EQ(correction.beatNear(0.75, 0.05), std::nullopt);
}

TEST_F(GridCorrectionTest, FKO02_ABeatMovesButStaysBetweenItsNeighbours)
{
    correction.moveBeat(2, 1.1);
    EXPECT_THAT(beats(), ElementsAre(0, 24000, 52800, 72000, 96000));
    EXPECT_TRUE(project.beatGrid().isEdited());
    EXPECT_EQ(project.beatGrid().chosenMeter(), 4);
    EXPECT_EQ(project.beatGrid().downbeatPhase(), 1);

    correction.moveBeat(2, 5.0);
    EXPECT_EQ(beats()[2], 72000 - 2400);
    correction.moveBeat(2, 0.0);
    EXPECT_EQ(beats()[2], 24000 + 2400);
    correction.moveBeat(0, -1.0);
    EXPECT_EQ(beats()[0], -48000);
    correction.moveBeat(4, 3.0);
    EXPECT_EQ(beats()[4], 144000);

    correction.moveBeat(5, 1.0);
    EXPECT_EQ(beats().size(), 5U);
}

TEST_F(GridCorrectionTest, FKO02_ABeatIsInsertedBetweenOthersButNotTooCloseToThem)
{
    correction.insertBeat(0.75);
    EXPECT_THAT(beats(), ElementsAre(0, 24000, 36000, 48000, 72000, 96000));
    EXPECT_EQ(project.beatGrid().downbeatPhase(), 1);

    correction.insertBeat(2.5);
    correction.insertBeat(-0.5);
    EXPECT_THAT(beats(), ElementsAre(-24000, 0, 24000, 36000, 48000, 72000, 96000, 120000));

    correction.insertBeat(0.98);
    correction.insertBeat(1.02);
    EXPECT_EQ(beats().size(), 8U);
}

TEST_F(GridCorrectionTest, FKO02_ABeatIsDeletedButTwoStay)
{
    correction.deleteBeat(1);
    EXPECT_THAT(beats(), ElementsAre(0, 48000, 72000, 96000));
    correction.deleteBeat(4);
    correction.deleteBeat(0);
    correction.deleteBeat(0);
    EXPECT_THAT(beats(), ElementsAre(72000, 96000));
    correction.deleteBeat(0);
    EXPECT_EQ(beats().size(), 2U);
}

TEST_F(GridCorrectionTest, FKO02_TheDownbeatIsSetOnABeat)
{
    correction.setDownbeat(3);
    EXPECT_EQ(project.beatGrid().downbeatPhase(), 3);
    correction.setDownbeat(4);
    EXPECT_EQ(project.beatGrid().downbeatPhase(), 0);
    correction.setDownbeat(5);
    EXPECT_EQ(project.beatGrid().downbeatPhase(), 0);
}

TEST_F(GridCorrectionTest, FKO02_WithoutAGridNothingCanBeCorrected)
{
    project.beatGrid().clear();

    correction.insertBeat(1.0);
    correction.setDownbeat(0);
    correction.chooseMeter(0);

    EXPECT_TRUE(project.beatGrid().beats().empty());
    EXPECT_TRUE(correction.meterChoices().empty());
    EXPECT_EQ(correction.beatNear(0.0, 1.0), std::nullopt);

    // Beats without a meter have no downbeat.
    project.beatGrid().setAnalysis({0, 24000}, {});
    correction.setDownbeat(0);
    EXPECT_FALSE(project.beatGrid().isEdited());
}

/// The riff of the PoC as transcription of a bass, as in RhythmUpdateTest.
class GridCorrectionWithNotesTest : public GridCorrectionTest
{
protected:
    void SetUp() override
    {
        folder = fs::path{"projects"} / "Probe";
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
        auto take = project.addVoice({.id = "v1", .name = "Bass", .profileId = "bass5-gs"})
                        .addTake({.id = "t1", .diFile = "audio/Bass DI.wav"});
        take.setNotes("audio/Bass DI.notes.csv", 105);
        take.setOnsets("audio/Bass DI.onsets.csv");
        files[*folder / "audio" / "Bass DI.notes.csv"] =
            transcription::notesToCsv(rhythm::test::readNotes("riff"));
        files[*folder / "audio" / "Bass DI.onsets.csv"] =
            rhythm::onsetsToCsv({.beat = rhythm::test::readFloats("riff-oenv441.f32"),
                                 .detail = rhythm::test::readFloats("riff-oenv256.f32")});
        correction.detectAgain();
        ASSERT_EQ(project.beatGrid().source(), "auto");
    }

    std::map<fs::path, std::string> files;
};

TEST_F(GridCorrectionWithNotesTest, FKO05_TheNotesFollowTheCorrectedGrid)
{
    const auto analysed = beats();
    const auto eventsBefore = files.at(*folder / "audio" / "Bass DI.events.csv");

    correction.deleteBeat(analysed.size() / 2);

    EXPECT_EQ(beats().size(), analysed.size() - 1);
    EXPECT_NE(files.at(*folder / "audio" / "Bass DI.events.csv"), eventsBefore);
    EXPECT_EQ(project.beatGrid().scoreFile(), "score.musicxml");
    EXPECT_TRUE(messages.empty());
}

TEST_F(GridCorrectionWithNotesTest, FKO02_DetectingAgainForgetsTheCorrections)
{
    const auto analysed = beats();
    correction.deleteBeat(1);

    correction.detectAgain();

    EXPECT_EQ(beats(), analysed);
    EXPECT_FALSE(project.beatGrid().isEdited());
}

TEST_F(GridCorrectionWithNotesTest, FKO02_ReportsNotesThatCannotBeWritten)
{
    EXPECT_CALL(fileSystem, writeText(_, _)).WillOnce(Return(false));

    correction.setDownbeat(0);

    ASSERT_EQ(messages.size(), 1U);
    EXPECT_EQ(messages.front(), i18n::tr("Writing the rhythm failed; is the disk full?"));
}

} // namespace
} // namespace snotes::ui
