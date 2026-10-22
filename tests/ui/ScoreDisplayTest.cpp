#include "ui/ScoreDisplay.h"

#include "model/ProjectFactory.h"
#include "notation/MusicXml.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace snotes::ui
{
namespace
{

namespace fs = std::filesystem;
using View = model::Project::View;

TEST(ScoreDisplayTest, FNA02_TheViewsShareTheWidth)
{
    EXPECT_EQ(viewWidths(View::tracks, 1000, 8), (ViewWidths{.tracks = 1000, .notes = 0}));
    EXPECT_EQ(viewWidths(View::notes, 1000, 8), (ViewWidths{.tracks = 0, .notes = 1000}));
    EXPECT_EQ(viewWidths(View::both, 1009, 8), (ViewWidths{.tracks = 501, .notes = 500}));
    EXPECT_NE((ViewWidths{.tracks = 1, .notes = 0}), (ViewWidths{.tracks = 1, .notes = 1}));
    EXPECT_NE((ViewWidths{.tracks = 0, .notes = 1}), (ViewWidths{.tracks = 1, .notes = 1}));
}

TEST(ScoreDisplayTest, FNA02_TheScoreIsInTheProjectFolder)
{
    const auto tree = model::ProjectFactory::createDefault("Probe");
    const fs::path folder{"Probe"};
    EXPECT_EQ(scorePath(tree, folder), std::nullopt);

    model::Project{tree, nullptr}.beatGrid().setScoreFile("score.musicxml");
    EXPECT_EQ(scorePath(tree, folder), folder / "score.musicxml");
    EXPECT_EQ(scorePath(tree, std::nullopt), std::nullopt);
}

TEST(ScoreDisplayTest, FNA01_PagesOfAScoreForJuce)
{
    notation::ScoreRenderer renderer{SNOTES_VEROVIO_DATA_DIR};
    const auto score = notation::buildScore(
        {{.name = "Bass", .clef = model::Clef::bass8vb, .fretboard = {.strings = {28, 33}, .frets = 20}}},
        {});

    const auto pages = scorePages(renderer, notation::toMusicXml(score), 800, "#ffffff");

    ASSERT_EQ(pages.size(), 1U);
    ASSERT_NE(pages.front(), nullptr);
    EXPECT_EQ(pages.front()->getStringAttribute("width"), "800px");
    EXPECT_TRUE(scorePages(renderer, "keine Noten", 800, "#ffffff").empty());
}

} // namespace
} // namespace snotes::ui
