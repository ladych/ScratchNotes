#include "transcription/NoteCreation.h"

#include "transcription/ReferenceData.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

namespace snotes::transcription
{
namespace
{

using ::testing::ElementsAre;
using ::testing::IsEmpty;

constexpr int kA2 = 45;
constexpr int kPitchA2 = kA2 - basicpitch::kLowestPitch;

/// Activations of a short recording that is silent except where the test sets values.
struct Grid
{
    explicit Grid(int numFrames)
    {
        activations.numFrames = numFrames;
        activations.frames.assign(Activations::index(numFrames, 0), 0.0F);
        activations.onsets.assign(activations.frames.size(), 0.0F);
    }

    void sound(int pitch, int from, int to, float level)
    {
        for (int frame = from; frame < to; ++frame)
            activations.frames[Activations::index(frame, pitch)] = level;
    }

    void onset(int pitch, int frame, float level)
    {
        activations.onsets[Activations::index(frame, pitch)] = level;
    }

    Activations activations;
};

NoteSettings plain()
{
    NoteSettings settings;
    settings.inferOnsets = false;
    settings.melodiaTrick = false;
    return settings;
}

void expectReference(const std::string& prefix)
{
    const auto expected = test::readNotes(prefix);
    const auto notes = createNotes(test::readActivations(prefix), NoteSettings{});

    ASSERT_EQ(notes.size(), expected.size());
    for (std::size_t index = 0; index < notes.size(); ++index)
    {
        SCOPED_TRACE(index);
        const auto& reference = expected[index];
        EXPECT_EQ(notes[index].startFrame, reference.note.startFrame);
        EXPECT_EQ(notes[index].endFrame, reference.note.endFrame);
        EXPECT_EQ(notes[index].pitch, reference.note.pitch);
        EXPECT_NEAR(notes[index].amplitude, reference.note.amplitude, 1e-5);
        EXPECT_NEAR(frameToSeconds(notes[index].startFrame), reference.startSeconds, 1e-6);
        EXPECT_NEAR(frameToSeconds(notes[index].endFrame), reference.endSeconds, 1e-6);
    }
}

TEST(NoteCreationTest, E12_GuitarNotesEqualThePythonReference)
{
    expectReference("clean");
}

TEST(NoteCreationTest, E12_BassNotesEqualThePythonReference)
{
    expectReference("bass");
}

TEST(NoteCreationTest, FTX02_NoteStartsAtTheOnsetPeakAndEndsWhenTheEnergyIsGone)
{
    Grid grid{60};
    grid.sound(kPitchA2, 10, 30, 0.8F);
    grid.onset(kPitchA2, 10, 0.9F);

    EXPECT_THAT(createNotes(grid.activations, plain()),
                ElementsAre(FrameNote{.startFrame = 10, .endFrame = 30, .pitch = kA2, .amplitude = 0.8F}));
}

TEST(NoteCreationTest, FTX02_ShortGapsDoNotEndANote)
{
    Grid grid{80};
    grid.sound(kPitchA2, 10, 20, 0.8F);
    grid.sound(kPitchA2, 25, 40, 0.8F);
    grid.onset(kPitchA2, 10, 0.9F);

    const auto notes = createNotes(grid.activations, plain());

    ASSERT_EQ(notes.size(), 1U);
    EXPECT_EQ(notes[0].endFrame, 40);
}

TEST(NoteCreationTest, FTX02_WeakOnsetsShortNotesAndOnsetsAtTheEdgesMakeNoNote)
{
    Grid grid{60};
    grid.sound(kPitchA2, 10, 30, 0.8F);
    grid.onset(kPitchA2, 10, 0.4F);
    grid.sound(kPitchA2 + 5, 10, 15, 0.8F);
    grid.onset(kPitchA2 + 5, 10, 0.9F);
    grid.sound(kPitchA2 + 10, 0, 60, 0.8F);
    grid.onset(kPitchA2 + 10, 0, 0.9F);
    grid.onset(kPitchA2 + 10, 59, 0.9F);

    EXPECT_THAT(createNotes(grid.activations, plain()), IsEmpty());
}

TEST(NoteCreationTest, FTX02_AnOnsetMustBeAPeakOverTime)
{
    Grid grid{60};
    grid.sound(kPitchA2, 10, 30, 0.8F);
    grid.onset(kPitchA2, 10, 0.9F);
    grid.onset(kPitchA2, 11, 0.9F);
    grid.sound(kPitchA2 + 5, 10, 30, 0.8F);
    grid.onset(kPitchA2 + 5, 10, 0.9F);
    grid.onset(kPitchA2 + 5, 9, 0.95F);

    const auto notes = createNotes(grid.activations, plain());

    ASSERT_EQ(notes.size(), 1U);
    EXPECT_EQ(notes[0].pitch, kA2 + 5);
    EXPECT_EQ(notes[0].startFrame, 9);
}

TEST(NoteCreationTest, FTX02_ANoteTakesTheEnergyOfItsNeighbourPitches)
{
    Grid grid{60};
    grid.sound(kPitchA2, 10, 30, 0.8F);
    grid.sound(kPitchA2 + 1, 10, 30, 0.7F);
    grid.onset(kPitchA2, 10, 0.9F);
    NoteSettings settings = plain();
    settings.melodiaTrick = true;

    EXPECT_EQ(createNotes(grid.activations, settings).size(), 1U);
}

TEST(NoteCreationTest, FTX02_MelodiaTrickFindsNotesWithoutOnset)
{
    Grid grid{60};
    grid.sound(kPitchA2, 20, 40, 0.6F);
    grid.sound(0, 10, 13, 0.6F);
    grid.sound(basicpitch::kNumPitches - 1, 45, 48, 0.6F);
    NoteSettings settings = plain();
    settings.melodiaTrick = true;

    EXPECT_THAT(createNotes(grid.activations, settings),
                ElementsAre(FrameNote{.startFrame = 20, .endFrame = 39, .pitch = kA2, .amplitude = 0.6F}));
}

TEST(NoteCreationTest, FTX02_MelodiaTrickStopsAtTheEdgesOfTheRecording)
{
    Grid grid{30};
    grid.sound(kPitchA2, 0, 30, 0.6F);
    NoteSettings settings = plain();
    settings.melodiaTrick = true;

    EXPECT_THAT(createNotes(grid.activations, settings),
                ElementsAre(FrameNote{.startFrame = 0, .endFrame = 28, .pitch = kA2, .amplitude = 0.6F}));
}

TEST(NoteCreationTest, FTX02_RisingNoteActivationCountsAsOnset)
{
    Grid grid{60};
    grid.sound(kPitchA2, 10, 30, 0.8F);
    grid.onset(kPitchA2 + 20, 40, 0.9F);
    NoteSettings settings;
    settings.melodiaTrick = false;

    EXPECT_THAT(createNotes(grid.activations, settings),
                ElementsAre(FrameNote{.startFrame = 10, .endFrame = 30, .pitch = kA2, .amplitude = 0.8F}));
}

TEST(NoteCreationTest, FTX02_SilenceHasNoNotes)
{
    EXPECT_THAT(createNotes(Grid{60}.activations, NoteSettings{}), IsEmpty());
    EXPECT_THAT(createNotes(Activations{}, NoteSettings{}), IsEmpty());
}

TEST(NoteCreationTest, FTX02_MinimumLengthInFramesAsBasicPitchRoundsIt)
{
    EXPECT_EQ(minimumFramesFor(80.0), 7);
    EXPECT_EQ(minimumFramesFor(127.7), 11);
}

TEST(NoteCreationTest, FTX02_FrameTimesFollowTheWindowsOfTheModel)
{
    EXPECT_DOUBLE_EQ(frameToSeconds(0), 0.0);
    EXPECT_NEAR(frameToSeconds(86), 86 * 256 / 22050.0, 1e-12);
    EXPECT_NEAR(frameToSeconds(172), (172 * 256 / 22050.0) - 0.0103261, 1e-6);
}

TEST(NoteCreationTest, FTX02_NotesDifferInEveryField)
{
    const FrameNote note{.startFrame = 1, .endFrame = 9, .pitch = 40, .amplitude = 0.5F};

    EXPECT_EQ(note, note);
    EXPECT_NE(note, (FrameNote{.startFrame = 2, .endFrame = 9, .pitch = 40, .amplitude = 0.5F}));
    EXPECT_NE(note, (FrameNote{.startFrame = 1, .endFrame = 8, .pitch = 40, .amplitude = 0.5F}));
    EXPECT_NE(note, (FrameNote{.startFrame = 1, .endFrame = 9, .pitch = 41, .amplitude = 0.5F}));
    EXPECT_NE(note, (FrameNote{.startFrame = 1, .endFrame = 9, .pitch = 40, .amplitude = 0.6F}));
}

} // namespace
} // namespace snotes::transcription
