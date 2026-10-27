#include "ui/ScoreDisplay.h"

#include "model/ProjectFactory.h"
#include "notation/MusicXml.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <optional>
#include <vector>

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

TEST(ScoreDisplayTest, FNA04_AreasUnite)
{
    const Area a{.left = 1.0F, .top = 2.0F, .right = 3.0F, .bottom = 4.0F};
    const Area b{.left = 0.0F, .top = 3.0F, .right = 5.0F, .bottom = 6.0F};

    EXPECT_EQ(unite(a, b), (Area{.left = 0.0F, .top = 2.0F, .right = 5.0F, .bottom = 6.0F}));
    EXPECT_EQ(unite(a, {}), a);
    EXPECT_EQ(unite({}, b), b);
    EXPECT_TRUE((Area{.left = 1.0F, .top = 0.0F, .right = 1.0F, .bottom = 2.0F}).empty());
    EXPECT_TRUE((Area{.left = 0.0F, .top = 1.0F, .right = 2.0F, .bottom = 1.0F}).empty());
    EXPECT_FALSE(a.empty());
    EXPECT_NE(a, (Area{.left = 0.0F, .top = 2.0F, .right = 3.0F, .bottom = 4.0F}));
    EXPECT_NE(a, (Area{.left = 1.0F, .top = 0.0F, .right = 3.0F, .bottom = 4.0F}));
    EXPECT_NE(a, (Area{.left = 1.0F, .top = 2.0F, .right = 0.0F, .bottom = 4.0F}));
    EXPECT_NE(a, (Area{.left = 1.0F, .top = 2.0F, .right = 3.0F, .bottom = 0.0F}));
}

TEST(ScoreDisplayTest, FNA04_TheCursorCoversTheNotesThatStartedLastInTheirBars)
{
    const Area firstBar{.left = 0.0F, .top = 10.0F, .right = 100.0F, .bottom = 50.0F};
    const Area secondPart{.left = 0.0F, .top = 60.0F, .right = 100.0F, .bottom = 100.0F};
    const std::vector<DrawnNote> notes{
        {.span = {.start = 0, .end = 12},
         .area = {.left = 10.0F, .top = 20.0F, .right = 18.0F, .bottom = 26.0F},
         .bar = firstBar},
        {.span = {.start = 12, .end = 24},
         .area = {.left = 40.0F, .top = 15.0F, .right = 48.0F, .bottom = 21.0F},
         .bar = firstBar},
        {.span = {.start = 12, .end = 18},
         .area = {.left = 42.0F, .top = 70.0F, .right = 50.0F, .bottom = 76.0F},
         .bar = secondPart},
        // A fret of the tablature, drawn without an area of its own.
        {.span = {.start = 12, .end = 24}, .area = {}, .bar = secondPart},
        {.span = {.start = 0, .end = 12}, .area = {}, .bar = firstBar}};
    const Area first{.left = 10.0F, .top = 10.0F, .right = 18.0F, .bottom = 50.0F};

    EXPECT_EQ(cursorArea(notes, -1.0), std::nullopt);
    EXPECT_EQ(cursorArea(notes, 0.0), first);
    EXPECT_EQ(cursorArea(notes, 11.5), first);
    EXPECT_EQ(cursorArea(notes, 20.0), (Area{.left = 40.0F, .top = 10.0F, .right = 50.0F, .bottom = 100.0F}));
    EXPECT_EQ(cursorArea(notes, 24.0), std::nullopt);
    EXPECT_EQ(cursorArea({}, 0.0), std::nullopt);
}

} // namespace
} // namespace snotes::ui
