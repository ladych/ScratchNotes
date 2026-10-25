#include "ui/BeatTapper.h"

#include "i18n/Translate.h"
#include "io/MockFileSystem.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <optional>

namespace snotes::ui
{
namespace
{

using ::testing::ElementsAre;
using ::testing::NiceMock;

/// A grid of a beat every second at 48 kHz, played back with 10 ms output latency.
class BeatTapperTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        project.beatGrid().setAnalysis({0, 48000, 96000, 144000, 192000},
                                       {{.numerator = 4, .confidence = 1.0, .phase = 0}});
    }

    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, nullptr};
    NiceMock<io::MockFileSystem> fileSystem;
    GridCorrection correction{tree,
                              fileSystem,
                              {.projectFolder = [] { return std::optional<std::filesystem::path>{}; },
                               .showMessage = [](const std::string&, const std::string&) {}}};
    double position = 0.0;
    bool playing = false;
    BeatTapper tapper{correction,
                      {.positionSeconds = [this] { return position; },
                       .isPlaying = [this] { return playing; },
                       .outputLatencySeconds = [] { return 0.01; }}};
};

TEST_F(BeatTapperTest, FKO03_TapsDuringPlaybackReplaceTheBeatsWhenItStops)
{
    playing = true;
    for (const double heard : {1.26, 1.76, 2.26})
    {
        position = heard;
        tapper.tap();
    }
    tapper.poll();
    EXPECT_FALSE(project.beatGrid().isEdited());
    EXPECT_EQ(tapper.statusText(),
              i18n::tr("Tapping the beat: %1 beats, they replace the grid when the playback stops", "3"));

    playing = false;
    tapper.poll();

    EXPECT_THAT(project.beatGrid().beats(), ElementsAre(0, 48000, 60000, 84000, 108000, 144000, 192000));
    EXPECT_EQ(tapper.statusText(), "");
    tapper.poll();
    EXPECT_EQ(project.beatGrid().beats().size(), 7U);
}

TEST_F(BeatTapperTest, FKO03_TapsWhileTheTransportStandsAreIgnored)
{
    position = 1.5;
    tapper.tap();
    tapper.tap();
    tapper.poll();

    EXPECT_EQ(tapper.statusText(), "");
    EXPECT_FALSE(project.beatGrid().isEdited());
}

} // namespace
} // namespace snotes::ui
