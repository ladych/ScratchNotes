#include "notation/ScoreMidi.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <memory>
#include <vector>

namespace snotes::notation
{
namespace
{

using ::testing::DoubleNear;
using ::testing::ElementsAre;

/// Beats at 0.5 s, 1.0 s, 1.5 s, then slower: 2.1 s and 2.8 s.
const std::vector<double> kBeats{0.5, 1.0, 1.5, 2.1, 2.8};

ScoreEvent note(std::int64_t start, std::int64_t duration, std::vector<int> pitches)
{
    ScoreEvent event;
    event.startTick = start;
    event.durationTicks = duration;
    event.chord.pitches = std::move(pitches);
    return event;
}

/// 2/4 from beat 1 of the grid with an upbeat of one beat: a guitar note on beat 1, a chord on beat 2.
Score twoFour()
{
    Score score;
    score.beatsPerBar = 2;
    score.pickupTicks = 12;
    score.bars = 1;
    score.originTick = 12;
    ScoreVoice guitar;
    guitar.name = "Gitarre";
    guitar.events = {note(0, 12, {40}), note(12, 6, {45, 52})};
    ScoreVoice bass;
    bass.name = "Bass";
    bass.clef = model::Clef::bass8vb;
    bass.events = {note(24, 12, {28})};
    bass.events.front().graces = {{.pitches = {26}, .shape = {}}, {.pitches = {27}, .shape = {}}};
    score.voices = {guitar, bass};
    return score;
}

std::unique_ptr<juce::MidiFile> read(const std::vector<std::uint8_t>& bytes)
{
    juce::MemoryInputStream stream{bytes.data(), bytes.size(), false};
    auto file = std::make_unique<juce::MidiFile>();
    if (!file->readFrom(stream))
        return nullptr;
    return file;
}

struct Played
{
    int pitch = 0;
    int channel = 0;
    double on = 0.0;
    double off = 0.0;
};

/// The notes of a track with their times in seconds.
std::vector<Played> notesOf(const juce::MidiMessageSequence& track)
{
    std::vector<Played> notes;
    for (const auto* event : track)
        if (event->message.isNoteOn())
            notes.push_back({.pitch = event->message.getNoteNumber(),
                             .channel = event->message.getChannel(),
                             .on = event->message.getTimeStamp(),
                             .off = event->noteOffObject->message.getTimeStamp()});
    return notes;
}

TEST(ScoreMidiTest, FEX01_BeatsBetweenAndAroundTheGrid)
{
    EXPECT_DOUBLE_EQ(secondsAt(kBeats, 0.0), 0.5);
    EXPECT_DOUBLE_EQ(secondsAt(kBeats, 2.5), 1.8);
    EXPECT_DOUBLE_EQ(secondsAt(kBeats, 4.0), 2.8);
    EXPECT_DOUBLE_EQ(secondsAt(kBeats, -1.0), 0.0);
    EXPECT_DOUBLE_EQ(secondsAt(kBeats, 5.5), 3.85);
}

TEST(ScoreMidiTest, FNA04_TheBeatAtATimeIsTheInverse)
{
    for (const double beat : {-1.0, 0.0, 0.25, 2.5, 4.0, 5.5})
        EXPECT_NEAR(beatAt(kBeats, secondsAt(kBeats, beat)), beat, 1e-12);
}

TEST(ScoreMidiTest, FEX01_ATypeOneFileWithATempoTrackAndATrackPerVoice)
{
    const auto file = read(scoreToMidi(twoFour(), kBeats));
    ASSERT_NE(file, nullptr);

    EXPECT_EQ(file->getTimeFormat(), kMidiTicksPerBeat);
    ASSERT_EQ(file->getNumTracks(), 3);
    const auto& tempoTrack = *file->getTrack(0);
    ASSERT_GT(tempoTrack.getNumEvents(), 0);
    int numerator = 0;
    int denominator = 0;
    tempoTrack.getEventPointer(0)->message.getTimeSignatureInfo(numerator, denominator);
    EXPECT_EQ(numerator, 2);
    EXPECT_EQ(denominator, 4);
    const auto* meter = tempoTrack.getEventPointer(0)->message.getRawData();
    EXPECT_EQ(meter[5], 24);
    EXPECT_EQ(meter[6], 8);
    EXPECT_EQ(file->getTrack(1)->getEventPointer(0)->message.getTextFromTextMetaEvent(), "Gitarre");
    EXPECT_EQ(file->getTrack(2)->getEventPointer(0)->message.getTextFromTextMetaEvent(), "Bass");
    EXPECT_EQ(file->getTrack(1)->getEventPointer(1)->message.getProgramChangeNumber(), 27);
    EXPECT_EQ(file->getTrack(2)->getEventPointer(1)->message.getProgramChangeNumber(), 33);
}

TEST(ScoreMidiTest, FEX01_NotesSoundWhereTheyWerePlayed)
{
    auto file = read(scoreToMidi(twoFour(), kBeats));
    ASSERT_NE(file, nullptr);
    file->convertTimestampTicksToSeconds();

    const auto guitar = notesOf(*file->getTrack(1));
    ASSERT_EQ(guitar.size(), 3U);
    EXPECT_EQ(guitar[0].pitch, 40);
    EXPECT_EQ(guitar[0].channel, 1);
    EXPECT_THAT(guitar[0].on, DoubleNear(1.0, 1e-4));
    EXPECT_THAT(guitar[0].off, DoubleNear(1.5, 1e-4));
    EXPECT_THAT(guitar[1].on, DoubleNear(1.5, 1e-4));
    EXPECT_THAT(guitar[2].off, DoubleNear(1.8, 1e-4));
    const auto bass = notesOf(*file->getTrack(2));
    ASSERT_EQ(bass.size(), 3U);
    EXPECT_EQ(bass[2].pitch, 28);
    EXPECT_EQ(bass[2].channel, 2);
    EXPECT_THAT(bass[2].on, DoubleNear(2.1, 1e-4));
    EXPECT_THAT(bass[2].off, DoubleNear(2.8, 1e-4));
}

TEST(ScoreMidiTest, FEX01_GraceNotesAreThirtySecondsJustBeforeTheirChord)
{
    const auto file = read(scoreToMidi(twoFour(), kBeats));
    ASSERT_NE(file, nullptr);

    const auto bass = notesOf(*file->getTrack(2));
    ASSERT_EQ(bass.size(), 3U);
    const auto chord = bass[2].on;
    EXPECT_EQ(bass[0].pitch, 26);
    EXPECT_DOUBLE_EQ(bass[0].on, chord - 240.0);
    EXPECT_DOUBLE_EQ(bass[0].off, chord - 120.0);
    EXPECT_EQ(bass[1].pitch, 27);
    EXPECT_DOUBLE_EQ(bass[1].off, chord);
}

/// The tempo events of the first track: tick and seconds per beat.
std::vector<std::pair<double, double>> temposOf(const juce::MidiFile& file)
{
    std::vector<std::pair<double, double>> tempos;
    for (const auto* event : *file.getTrack(0))
        if (event->message.isTempoMetaEvent())
            tempos.emplace_back(event->message.getTimeStamp(),
                                event->message.getTempoSecondsPerQuarterNote());
    return tempos;
}

TEST(ScoreMidiTest, FEX01_BarOneOfTheScoreFallsOnABarLine)
{
    const auto file = read(scoreToMidi(twoFour(), kBeats));
    ASSERT_NE(file, nullptr);

    // 1 s before the upbeat at 0.5 s per beat: two bars of lead-in, minus the upbeat, are three beats.
    const auto tempos = temposOf(*file);
    ASSERT_EQ(tempos.size(), 4U);
    EXPECT_DOUBLE_EQ(tempos[0].first, 0.0);
    EXPECT_THAT(tempos[0].second, DoubleNear(1.0 / 3.0, 1e-6));
    EXPECT_DOUBLE_EQ(tempos[1].first, 2880.0);
    EXPECT_THAT(tempos[1].second, DoubleNear(0.5, 1e-6));
    EXPECT_DOUBLE_EQ(tempos[2].first, 3840.0);
    EXPECT_THAT(tempos[2].second, DoubleNear(0.6, 1e-6));
    EXPECT_THAT(tempos[3].second, DoubleNear(0.7, 1e-6));
    // Bar 1 starts after the upbeat, at four beats: a bar line in 2/4.
    EXPECT_DOUBLE_EQ(std::fmod(tempos[2].first, 2.0 * kMidiTicksPerBeat), 0.0);
}

TEST(ScoreMidiTest, FEX01_ALongLeadInTakesWholeBarsCloseToTheTempo)
{
    auto score = twoFour();
    score.pickupTicks = 0;
    score.originTick = 36;
    const std::vector<double> beats{0.6, 1.1, 1.6, 2.1, 2.6, 3.1};

    const auto file = read(scoreToMidi(score, beats));
    ASSERT_NE(file, nullptr);

    // 2.1 s at 0.5 s per beat are 4.2 beats: two bars of 2/4 at 0.525 s per beat.
    const auto tempos = temposOf(*file);
    ASSERT_FALSE(tempos.empty());
    EXPECT_THAT(tempos[0].second, DoubleNear(0.525, 1e-6));
    EXPECT_DOUBLE_EQ(tempos[1].first, 4.0 * kMidiTicksPerBeat);
}

TEST(ScoreMidiTest, FEX01_ChannelsSkipTheDrums)
{
    Score score;
    score.voices.resize(11);
    score.voices.back().events = {note(0, 12, {40})};

    const auto file = read(scoreToMidi(score, kBeats));
    ASSERT_NE(file, nullptr);

    ASSERT_EQ(file->getNumTracks(), 12);
    std::vector<int> channels;
    for (int track = 9; track < 12; ++track)
        channels.push_back(file->getTrack(track)->getEventPointer(1)->message.getChannel());
    EXPECT_THAT(channels, ElementsAre(9, 11, 12));
}

} // namespace
} // namespace snotes::notation
