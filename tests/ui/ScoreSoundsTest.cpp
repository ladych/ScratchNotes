#include "ui/ScoreSounds.h"

#include "io/MockFileSystem.h"
#include "model/ModelIds.h"
#include "model/Project.h"
#include "model/ProjectFactory.h"
#include "notation/Score.h"
#include "notation/ScoreMidi.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{
namespace
{

namespace fs = std::filesystem;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

/// Beats every half second from 1 s on.
const std::vector<double> kBeats{1.0, 1.5, 2.0, 2.5, 3.0};

notation::ScoreEvent event(std::int64_t start, std::int64_t duration, int pitch)
{
    return {.startTick = start,
            .durationTicks = duration,
            .chord = {.pitches = {pitch}, .shape = {}},
            .graces = {}};
}

/// A bar of bass, A2 on the first beat and E2 on the third, and a bar of guitar without notes.
std::string scoreMidi()
{
    const notation::Score score{
        .beatsPerBar = 4,
        .pickupTicks = 0,
        .bars = 1,
        .voices = {{.name = "Bass", .events = {event(0, 12, 45), event(24, 6, 40)}}, {.name = "Gitarre"}},
        .originTick = 0,
        .bpm = 120.0};
    const auto bytes = notation::scoreToMidi(score, kBeats);
    return {bytes.begin(), bytes.end()};
}

TEST(ScoreSoundsTest, FNA05_TheNotesOfTheMidiFileInSamplesOfTheSong)
{
    const auto voices = midiVoices(scoreMidi(), 1000.0);

    ASSERT_EQ(voices.size(), 1U);
    EXPECT_EQ(voices[0].name, "Bass");
    ASSERT_NE(voices[0].notes, nullptr);
    const auto& notes = voices[0].notes->notes;
    ASSERT_EQ(notes.size(), 2U);
    EXPECT_NEAR(static_cast<double>(notes[0].start), 1000.0, 1.0);
    EXPECT_NEAR(static_cast<double>(notes[0].end), 1500.0, 1.0);
    EXPECT_NEAR(notes[0].frequency, 110.0, 1e-9);
    EXPECT_NEAR(static_cast<double>(notes[1].start), 2000.0, 1.0);
    EXPECT_NEAR(static_cast<double>(notes[1].end), 2250.0, 1.0);
    EXPECT_EQ(voices[0].notes->longest, notes[0].end - notes[0].start);
    EXPECT_TRUE(midiVoices("keine Noten", 1000.0).empty());
}

TEST(ScoreSoundsTest, FNA05_ANoteThatNeverEndsDoesNotSound)
{
    juce::MidiMessageSequence track;
    track.addEvent(juce::MidiMessage::noteOn(1, 40, static_cast<juce::uint8>(100)), 0.0);
    juce::MidiFile file;
    file.setTicksPerQuarterNote(960);
    file.addTrack(track);
    juce::MemoryOutputStream stream;
    file.writeTo(stream);

    EXPECT_TRUE(midiVoices(stream.toString().toStdString(), 1000.0).empty());
}

class ScoreSoundsCacheTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        project.beatGrid().setScoreFile("score.musicxml");
        ON_CALL(fileSystem, readText(folder / "score.mid")).WillByDefault(Return(scoreMidi()));
    }

    fs::path folder{"Probe"};
    std::optional<fs::path> currentFolder = folder;
    juce::ValueTree tree = model::ProjectFactory::createDefault("Probe");
    model::Project project{tree, nullptr};
    NiceMock<io::MockFileSystem> fileSystem;
    ScoreSounds sounds{fileSystem, [this] { return currentFolder; }};
};

TEST_F(ScoreSoundsCacheTest, FNA05_ReadsTheMidiFileOfTheScoreAndConvertsItOnlyWhenItChanged)
{
    const auto first = sounds.voices(tree);
    ASSERT_EQ(first.size(), 1U);
    EXPECT_NEAR(static_cast<double>(first[0].notes->notes[0].start), 48000.0, 48.0);
    // The same notes are shared while the file stays.
    EXPECT_EQ(sounds.voices(tree)[0].notes, first[0].notes);

    tree.setProperty(model::ids::sampleRate, 44100.0, nullptr);
    const auto resampled = sounds.voices(tree);
    EXPECT_NE(resampled[0].notes, first[0].notes);
    EXPECT_NEAR(static_cast<double>(resampled[0].notes->notes[0].start), 44100.0, 45.0);

    ON_CALL(fileSystem, readText(folder / "score.mid")).WillByDefault(Return(std::string{"keine Noten"}));
    EXPECT_TRUE(sounds.voices(tree).empty());
}

TEST_F(ScoreSoundsCacheTest, FNA05_NoNotesWithoutFolderScoreOrFile)
{
    ON_CALL(fileSystem, readText(_)).WillByDefault(Return(std::nullopt));
    EXPECT_TRUE(sounds.voices(tree).empty());

    currentFolder.reset();
    EXPECT_TRUE(sounds.voices(tree).empty());

    currentFolder = folder;
    project.beatGrid().setScoreFile("");
    EXPECT_TRUE(sounds.voices(tree).empty());
}

} // namespace
} // namespace snotes::ui
