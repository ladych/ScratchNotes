#include "rhythm/CleanUp.h"

#include "rhythm/OnsetStrength.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

namespace snotes::rhythm
{
namespace
{

using ::testing::ElementsAre;
using transcription::Note;

Note note(double start, double seconds, int pitch, float amplitude = 0.8F)
{
    return {.startSeconds = start, .endSeconds = start + seconds, .pitch = pitch, .amplitude = amplitude};
}

/// Two seconds of onset strength with a clear attack at 1 s only.
std::vector<float> attackAtOneSecond()
{
    std::vector<float> envelope(static_cast<std::size_t>(2.0 * kAnalysisRate / kDetailHop) + 1);
    envelope[static_cast<std::size_t>(std::lround(1.0 * kAnalysisRate / kDetailHop))] = 1.0F;
    return envelope;
}

const std::vector<double> kBeats{0.5, 1.0, 1.5, 2.0};

std::vector<Cluster> cleaned(const std::vector<Note>& notes, Style style)
{
    const auto rules = rulesFor(style);
    return cleanUp(clusterOnsets(notes, rules.relativeAmplitude), attackAtOneSecond(), kBeats, rules);
}

std::vector<std::vector<int>> pitchesOf(const std::vector<Cluster>& clusters)
{
    std::vector<std::vector<int>> pitches;
    for (const auto& cluster : clusters)
        pitches.push_back(cluster.pitches);
    return pitches;
}

TEST(CleanUpTest, FTX07_RiffMeltsANoteWithoutAttackIntoTheChord)
{
    const auto clusters = cleaned({note(1.0, 0.5, 40), note(1.1, 0.4, 45)}, Style::riff);

    ASSERT_EQ(clusters.size(), 1U);
    EXPECT_THAT(clusters[0].pitches, ElementsAre(40, 45));
    EXPECT_DOUBLE_EQ(clusters[0].seconds, 1.0);
}

TEST(CleanUpTest, FTX07_MelodyKeepsItAsLegatoNote)
{
    EXPECT_THAT(pitchesOf(cleaned({note(1.0, 0.5, 40), note(1.1, 0.4, 45)}, Style::melody)),
                ElementsAre(ElementsAre(40), ElementsAre(45)));
}

TEST(CleanUpTest, FTX07_MelodyDropsALowResonance)
{
    EXPECT_THAT(pitchesOf(cleaned({note(1.0, 0.5, 40), note(1.1, 0.4, 33)}, Style::melody)),
                ElementsAre(ElementsAre(40)));
}

TEST(CleanUpTest, FTX07_DropsOvertonesOfARingingNote)
{
    EXPECT_THAT(pitchesOf(cleaned({note(1.0, 0.5, 40), note(1.1, 0.4, 52)}, Style::riff)),
                ElementsAre(ElementsAre(40)));
}

TEST(CleanUpTest, FTX07_LaterNotesWithoutAttackStay)
{
    // Too late to belong to the chord, and the chord no longer rings.
    EXPECT_THAT(pitchesOf(cleaned({note(1.0, 0.1, 40), note(1.3, 0.4, 52)}, Style::riff)),
                ElementsAre(ElementsAre(40), ElementsAre(52)));
}

TEST(CleanUpTest, FTX07_DropsLightNotesWithoutAttack)
{
    EXPECT_THAT(pitchesOf(cleaned({note(1.0, 0.5, 40), note(1.3, 0.1, 45)}, Style::riff)),
                ElementsAre(ElementsAre(40)));
}

TEST(CleanUpTest, FTX07_ALightNoteJustBeforeAHeavyOneIsItsGraceNote)
{
    const auto rules = rulesFor(Style::riff);
    const std::vector<float> flat(200, 0.1F);
    const auto clusters =
        cleanUp(clusterOnsets({note(0.9, 0.1, 38), note(1.0, 0.5, 40)}, 0.5), flat, kBeats, rules);

    ASSERT_EQ(clusters.size(), 1U);
    EXPECT_THAT(clusters[0].graces, ElementsAre(ElementsAre(38)));

    const auto heavyFirst =
        cleanUp(clusterOnsets({note(0.9, 0.5, 38), note(1.0, 0.2, 40)}, 0.5), flat, kBeats, rules);
    EXPECT_EQ(heavyFirst.size(), 2U);
}

TEST(CleanUpTest, FTX07_NotesAfterTheOnsetsHaveNoAttack)
{
    const auto clusters = cleanUp(
        clusterOnsets({note(100.0, 0.5, 40)}, 0.5), attackAtOneSecond(), kBeats, rulesFor(Style::riff));

    ASSERT_EQ(clusters.size(), 1U);
    EXPECT_DOUBLE_EQ(clusters[0].flux, 0.0);
}

TEST(CleanUpTest, FTX07_StyleFromTheShareOfChords)
{
    EXPECT_EQ(guessStyle({}), StyleGuess{});
    EXPECT_EQ(guessStyle({note(1.0, 0.5, 40), note(1.0, 0.5, 47), note(2.0, 0.5, 40)}),
              (StyleGuess{.style = Style::riff, .chordShare = 0.5}));
    EXPECT_NE(StyleGuess{.style = Style::riff}, StyleGuess{.style = Style::melody});
    EXPECT_NE(StyleGuess{.chordShare = 0.1}, StyleGuess{.chordShare = 0.2});
}

TEST(CleanUpTest, FTX07_RulesOfThePoc)
{
    EXPECT_TRUE(rulesFor(Style::riff).mergeLateNotes);
    EXPECT_DOUBLE_EQ(rulesFor(Style::riff).graceBeats, 0.35);
    EXPECT_FALSE(rulesFor(Style::melody).mergeLateNotes);
    EXPECT_DOUBLE_EQ(rulesFor(Style::melody).graceWeightRatio, 0.5);
}

} // namespace
} // namespace snotes::rhythm
