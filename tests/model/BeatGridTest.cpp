#include "model/BeatGrid.h"

#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace snotes::model
{
namespace
{

using ::testing::ElementsAre;

class BeatGridTest : public ::testing::Test
{
protected:
    juce::ValueTree tree = ProjectFactory::createDefault("Probe");
    juce::UndoManager undoManager;
    Project project{tree, &undoManager};
};

TEST_F(BeatGridTest, FTX05_NewProjectHasNoGrid)
{
    const auto grid = project.beatGrid();

    EXPECT_TRUE(grid.beats().empty());
    EXPECT_TRUE(grid.meterReadings().empty());
    EXPECT_EQ(grid.chosenMeter(), 0);
    EXPECT_EQ(grid.downbeatPhase(), 0);
    EXPECT_EQ(grid.source(), "");
}

TEST_F(BeatGridTest, FTX05_StoresTheAnalysisWithoutUndo)
{
    auto grid = project.beatGrid();

    grid.setAnalysis(
        {1000, 23050, 45100},
        {{.numerator = 3, .confidence = 0.6, .phase = 2}, {.numerator = 4, .confidence = 0.4, .phase = 0}});

    EXPECT_THAT(grid.beats(), ElementsAre(1000, 23050, 45100));
    EXPECT_EQ(grid.tree()[ids::beats].toString(), "1000 23050 45100");
    EXPECT_EQ(grid.source(), "auto");
    EXPECT_THAT(grid.meterReadings(),
                ElementsAre(MeterReading{.numerator = 3, .confidence = 0.6, .phase = 2},
                            MeterReading{.numerator = 4, .confidence = 0.4, .phase = 0}));
    EXPECT_EQ(grid.chosenMeter(), 3);
    EXPECT_EQ(grid.downbeatPhase(), 2);
    EXPECT_FALSE(undoManager.canUndo());
}

TEST_F(BeatGridTest, FTX06_ANewAnalysisReplacesTheOldOne)
{
    auto grid = project.beatGrid();
    grid.setAnalysis({1, 2}, {{.numerator = 3, .confidence = 1.0, .phase = 1}});

    grid.setAnalysis({5, 6, 7}, {});

    EXPECT_THAT(grid.beats(), ElementsAre(5, 6, 7));
    EXPECT_TRUE(grid.meterReadings().empty());
    EXPECT_EQ(grid.chosenMeter(), 0);

    grid.clear();
    EXPECT_TRUE(project.beatGrid().beats().empty());
    EXPECT_EQ(project.beatGrid().source(), "");
}

TEST_F(BeatGridTest, FNA02_KeepsTheScoreUntilTheGridIsCleared)
{
    auto grid = project.beatGrid();
    grid.setAnalysis({1, 2}, {{.numerator = 3, .confidence = 1.0, .phase = 1}});
    EXPECT_EQ(grid.scoreFile(), "");

    grid.setScoreFile("score.musicxml");
    EXPECT_EQ(grid.scoreFile(), "score.musicxml");
    EXPECT_FALSE(undoManager.canUndo());

    grid.clear();
    EXPECT_EQ(grid.scoreFile(), "");
}

TEST_F(BeatGridTest, FTX06_ReadingsDifferInEveryField)
{
    const MeterReading reading{.numerator = 4, .confidence = 0.5, .phase = 1};

    EXPECT_NE(reading, (MeterReading{.numerator = 3, .confidence = 0.5, .phase = 1}));
    EXPECT_NE(reading, (MeterReading{.numerator = 4, .confidence = 0.6, .phase = 1}));
    EXPECT_NE(reading, (MeterReading{.numerator = 4, .confidence = 0.5, .phase = 0}));
}

} // namespace
} // namespace snotes::model
