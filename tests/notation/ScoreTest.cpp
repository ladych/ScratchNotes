#include "notation/Score.h"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace snotes::notation
{
namespace
{

const tab::Fretboard kBass{.strings = {28, 33, 38, 43}, .frets = 24};

rhythm::QuantizedEvent event(rhythm::Fraction position, rhythm::Fraction duration, std::vector<int> pitches)
{
    rhythm::QuantizedEvent result;
    result.position = position;
    result.duration = duration;
    result.pitches = std::move(pitches);
    return result;
}

/// A bass voice with an open E for each position, a quarter note long.
VoiceEvents bass(const std::vector<rhythm::Fraction>& positions)
{
    VoiceEvents voice{
        .name = "Bass", .clef = model::Clef::bass8vb, .fretboard = kBass, .events = {}, .tabs = {}};
    for (const auto& position : positions)
    {
        voice.events.push_back(event(position, {1}, {28}));
        voice.tabs.push_back({.notes = {{.string = 0, .fret = 0}}, .graces = {}});
    }
    return voice;
}

TEST(ScoreTest, FNA02_WithoutNotesTheScoreIsOneEmptyBar)
{
    EXPECT_EQ(buildScore({}, {.beatsPerBar = 3, .phase = 2}),
              (Score{.beatsPerBar = 3, .pickupTicks = 0, .bars = 1, .voices = {}, .originTick = 24}));
    const auto score = buildScore({bass({})}, {});
    EXPECT_EQ(score.bars, 1);
    ASSERT_EQ(score.voices.size(), 1U);
    EXPECT_TRUE(score.voices.front().events.empty());
}

TEST(ScoreTest, FNA02_AScoreThatStartsOnADownbeatHasNoUpbeat)
{
    const auto score = buildScore({bass({{1}, {2}, {3}, {4}, {5}})}, {.beatsPerBar = 4, .phase = 1});

    EXPECT_EQ(score.pickupTicks, 0);
    EXPECT_EQ(score.bars, 2);
    EXPECT_EQ(score.barTicks(), 48);
    ASSERT_EQ(score.voices.size(), 1U);
    const auto& voice = score.voices.front();
    EXPECT_EQ(voice.name, "Bass");
    EXPECT_EQ(voice.clef, model::Clef::bass8vb);
    EXPECT_EQ(voice.fretboard, kBass);
    ASSERT_EQ(voice.events.size(), 5U);
    EXPECT_EQ(voice.events[0].startTick, 0);
    EXPECT_EQ(voice.events[4].startTick, 48);
    EXPECT_EQ(voice.events[4].durationTicks, 12);
    EXPECT_EQ(voice.events[4].chord, (ScoreChord{.pitches = {28}, .shape = {{.string = 0, .fret = 0}}}));
}

TEST(ScoreTest, FNA02_SilenceBeforeTheFirstNoteIsCutAndNotesBeforeTheFirstDownbeatAreAnUpbeat)
{
    // 3/4 with bars from beat 0: the first note on beat 2 of the fourth bar, after nine beats of silence.
    const auto score = buildScore({bass({{10}, {12}})}, {.beatsPerBar = 3, .phase = 0});

    EXPECT_EQ(score.pickupTicks, 24);
    EXPECT_EQ(score.bars, 1);
    EXPECT_EQ(score.originTick, 120);
    EXPECT_EQ(score.voices.front().events[0].startTick, 0);
    EXPECT_EQ(score.voices.front().events[1].startTick, 24);
}

TEST(ScoreTest, FNA02_AnUpbeatStartsOnTheBeatOfTheFirstNote)
{
    // 4/4 with the first downbeat on beat 2; the first note an eighth after beat 0.
    const auto score = buildScore({bass({{1, 2}, {2}})}, {.beatsPerBar = 4, .phase = 2});

    EXPECT_EQ(score.pickupTicks, 24);
    EXPECT_EQ(score.voices.front().events[0].startTick, 6);
    EXPECT_EQ(score.voices.front().events[1].startTick, 24);
}

TEST(ScoreTest, FNA02_ANoteJustAfterTheDownbeatStartsBarOne)
{
    const auto score = buildScore({bass({{9, 4}})}, {.beatsPerBar = 4, .phase = 6});

    EXPECT_EQ(score.pickupTicks, 0);
    EXPECT_EQ(score.voices.front().events[0].startTick, 3);
}

TEST(ScoreTest, FNA02_AllVoicesShareTheBarsOfTheEarliestVoice)
{
    auto guitar = bass({{6}});
    guitar.name = "Gitarre";
    const auto score = buildScore({guitar, bass({{3}})}, {.beatsPerBar = 4, .phase = 0});

    EXPECT_EQ(score.pickupTicks, 12);
    EXPECT_EQ(score.bars, 1);
    EXPECT_EQ(score.voices[0].name, "Gitarre");
    EXPECT_EQ(score.voices[0].events[0].startTick, 36);
    EXPECT_EQ(score.voices[1].events[0].startTick, 0);
}

TEST(ScoreTest, FNA01_ANoteEndsWhereTheNextOneStarts)
{
    auto voice = bass({{0}, {1, 2}});
    voice.events[0].duration = {2};
    voice.events[1].duration = {0};

    const auto score = buildScore({voice}, {.beatsPerBar = 4, .phase = 0});

    EXPECT_EQ(score.voices.front().events[0].durationTicks, 6);
    EXPECT_EQ(score.voices.front().events[1].durationTicks, 1);
}

TEST(ScoreTest, FNA01_GraceNotesKeepTheirTablature)
{
    auto voice = bass({{0}});
    voice.events[0].graces = {{30}, {31, 33}};
    voice.tabs[0].graces = {{{.string = 0, .fret = 2}}, {{.string = 0, .fret = 3}, {.string = 1, .fret = 0}}};

    const auto score = buildScore({voice}, {.beatsPerBar = 4, .phase = 0});

    const std::vector<ScoreChord> graces{
        {.pitches = {30}, .shape = {{.string = 0, .fret = 2}}},
        {.pitches = {31, 33}, .shape = {{.string = 0, .fret = 3}, {.string = 1, .fret = 0}}}};
    EXPECT_EQ(score.voices.front().events[0].graces, graces);
}

TEST(ScoreTest, FNA02_ScoresDifferInEveryPart)
{
    const Score score{.beatsPerBar = 4, .pickupTicks = 0, .bars = 1, .voices = {{}}};
    auto other = score;
    EXPECT_EQ(score, other);
    other.beatsPerBar = 3;
    EXPECT_NE(score, other);
    other = score;
    other.pickupTicks = 12;
    EXPECT_NE(score, other);
    other = score;
    other.bars = 2;
    EXPECT_NE(score, other);
    other = score;
    other.voices.clear();
    EXPECT_NE(score, other);
    other = score;
    other.originTick = 12;
    EXPECT_NE(score, other);
    other = score;
    other.bpm = 90.0;
    EXPECT_NE(score, other);
}

TEST(ScoreTest, FNA02_VoicesAndEventsDifferInEveryPart)
{
    const ScoreVoice voice{.name = "Bass", .clef = model::Clef::bass8vb, .fretboard = kBass, .events = {{}}};
    auto other = voice;
    EXPECT_EQ(voice, other);
    other.name = "Gitarre";
    EXPECT_NE(voice, other);
    other = voice;
    other.clef = model::Clef::treble8vb;
    EXPECT_NE(voice, other);
    other = voice;
    other.fretboard.frets = 21;
    EXPECT_NE(voice, other);
    other = voice;
    other.events.clear();
    EXPECT_NE(voice, other);

    const ScoreEvent event{
        .startTick = 0, .durationTicks = 12, .chord = {.pitches = {28}, .shape = {}}, .graces = {}};
    auto changed = event;
    EXPECT_EQ(event, changed);
    changed.startTick = 1;
    EXPECT_NE(event, changed);
    changed = event;
    changed.durationTicks = 6;
    EXPECT_NE(event, changed);
    changed = event;
    changed.chord.pitches = {29};
    EXPECT_NE(event, changed);
    changed = event;
    changed.chord.shape = {{}};
    EXPECT_NE(event, changed);
    changed = event;
    changed.graces = {{}};
    EXPECT_NE(event, changed);
}

} // namespace
} // namespace snotes::notation
