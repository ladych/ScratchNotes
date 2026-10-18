#include "transcription/NoteMidi.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <gtest/gtest.h>

namespace snotes::transcription
{
namespace
{

juce::MidiFile readBack(const std::vector<std::uint8_t>& bytes)
{
    juce::MemoryInputStream stream{bytes.data(), bytes.size(), false};
    juce::MidiFile file;
    EXPECT_TRUE(file.readFrom(stream));
    file.convertTimestampTicksToSeconds();
    return file;
}

TEST(NoteMidiTest, FTX02_KeepsTimesPitchesAndVelocities)
{
    const std::vector<Note> notes{{.startSeconds = 0.5, .endSeconds = 1.25, .pitch = 40, .amplitude = 1.0F},
                                  {.startSeconds = 1.0, .endSeconds = 2.0, .pitch = 52, .amplitude = 0.0F}};

    const auto file = readBack(notesToMidi(notes));

    ASSERT_EQ(file.getNumTracks(), 1);
    const auto& track = *file.getTrack(0);
    // Tempo, four note events and the end of the track.
    ASSERT_EQ(track.getNumEvents(), 6);
    EXPECT_TRUE(track.getEventPointer(0)->message.isTempoMetaEvent());
    const auto& first = track.getEventPointer(1)->message;
    EXPECT_TRUE(first.isNoteOn());
    EXPECT_EQ(first.getNoteNumber(), 40);
    EXPECT_EQ(first.getVelocity(), 127);
    EXPECT_DOUBLE_EQ(first.getTimeStamp(), 0.5);
    const auto& second = track.getEventPointer(2)->message;
    EXPECT_EQ(second.getNoteNumber(), 52);
    EXPECT_EQ(second.getVelocity(), 1);
    EXPECT_DOUBLE_EQ(second.getTimeStamp(), 1.0);
    EXPECT_DOUBLE_EQ(track.getEventPointer(3)->message.getTimeStamp(), 1.25);
    EXPECT_DOUBLE_EQ(track.getEventPointer(4)->message.getTimeStamp(), 2.0);
    EXPECT_TRUE(track.getEventPointer(5)->message.isEndOfTrackMetaEvent());
}

TEST(NoteMidiTest, FTX02_NoNotesGiveAnEmptyTrack)
{
    const auto file = readBack(notesToMidi({}));

    ASSERT_EQ(file.getNumTracks(), 1);
    // Only tempo and end of the track.
    EXPECT_EQ(file.getTrack(0)->getNumEvents(), 2);
}

} // namespace
} // namespace snotes::transcription
