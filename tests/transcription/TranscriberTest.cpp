#include "transcription/Transcriber.h"

#include "transcription/OnnxPitchModel.h"
#include "transcription/ReferenceData.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <vector>

namespace snotes::transcription
{
namespace
{

using namespace basicpitch;
using ::testing::ElementsAre;
using ::testing::IsEmpty;

constexpr double kRate = 22050.0;
/// The first frames of a window that are kept (half the overlap is dropped).
constexpr int kFirstKept = kOverlapFrames / 2;

struct FakeNote
{
    int pitchIndex = 0;
    /// Frames inside the window.
    int from = 0;
    int to = 0;
};

/// Every window holds the same notes, each with an onset on its first frame.
class FakeModel : public IPitchModel
{
public:
    explicit FakeModel(std::vector<FakeNote> notes) : notes_(std::move(notes)) {}

    void run(std::span<const float> /*window*/, std::span<float> frames, std::span<float> onsets) override
    {
        std::ranges::fill(frames, 0.0F);
        std::ranges::fill(onsets, 0.0F);
        for (const auto& note : notes_)
        {
            for (int frame = note.from; frame < note.to; ++frame)
                frames[Activations::index(frame, note.pitchIndex)] = 0.8F;
            onsets[Activations::index(note.from, note.pitchIndex)] = 0.9F;
        }
    }

private:
    std::vector<FakeNote> notes_;
};

bool always(double /*done*/)
{
    return true;
}

TranscriptionSettings guitar()
{
    return settingsFor(*model::findInstrumentProfile("guitar6-e"));
}

std::vector<float> oneSecond()
{
    return std::vector<float>(static_cast<std::size_t>(kRate), 0.5F);
}

Note expected(int startFrame, int endFrame, int pitch)
{
    return {.startSeconds = frameToSeconds(startFrame),
            .endSeconds = frameToSeconds(endFrame),
            .pitch = pitch,
            .amplitude = 0.8F};
}

TEST(TranscriberTest, FTX08_RangeOfTheInstrumentFromTheProfile)
{
    const auto bass = settingsFor(*model::findInstrumentProfile("bass5-gs"));
    const auto seven = settingsFor(*model::findInstrumentProfile("guitar7-gs"));

    EXPECT_TRUE(bass.octaveUp);
    EXPECT_EQ(bass.lowestPitch, 20);
    EXPECT_EQ(bass.highestPitch, 40 + 24);
    EXPECT_FALSE(seven.octaveUp);
    EXPECT_EQ(seven.lowestPitch, 32);
    EXPECT_EQ(seven.highestPitch, 61 + 24);
}

TEST(TranscriberTest, FTX02_FindsTheNotesOfTheModelInSeconds)
{
    FakeModel model{{{.pitchIndex = 31, .from = 30, .to = 100}}};

    const auto notes = transcribe(oneSecond(), kRate, model, guitar(), always);

    ASSERT_TRUE(notes.has_value());
    EXPECT_THAT(*notes, ElementsAre(expected(30 - kFirstKept, 100 - kFirstKept, 31 + kLowestPitch)));
}

TEST(TranscriberTest, FTX02_ResamplesToTheRateOfTheModel)
{
    FakeModel model{{{.pitchIndex = 31, .from = 30, .to = 100}}};

    const auto notes = transcribe(std::vector<float>(48000, 0.5F), 48000.0, model, guitar(), always);

    ASSERT_TRUE(notes.has_value());
    EXPECT_THAT(*notes, ElementsAre(expected(30 - kFirstKept, 100 - kFirstKept, 31 + kLowestPitch)));
}

TEST(TranscriberTest, FTX03_BassIsAnalysedAnOctaveHigher)
{
    FakeModel model{{{.pitchIndex = 31, .from = 30, .to = 100}}};
    const auto bass = settingsFor(*model::findInstrumentProfile("bass4-e"));

    const auto notes = transcribe(oneSecond(), kRate, model, bass, always);

    // Half a second at the model rate: 43 frames, the note runs to the last frame the decoder looks at.
    ASSERT_TRUE(notes.has_value());
    ASSERT_EQ(notes->size(), 1U);
    EXPECT_DOUBLE_EQ((*notes)[0].startSeconds, 2.0 * frameToSeconds(30 - kFirstKept));
    EXPECT_DOUBLE_EQ((*notes)[0].endSeconds, 2.0 * frameToSeconds(42));
    EXPECT_EQ((*notes)[0].pitch, 31 + kLowestPitch - 12);
}

TEST(TranscriberTest, FTX08_DropsNotesOutsideTheInstrument)
{
    FakeModel model{
        {{.pitchIndex = 0, .from = 30, .to = 100}, {.pitchIndex = kNumPitches - 1, .from = 30, .to = 100}}};

    const auto notes = transcribe(oneSecond(), kRate, model, guitar(), always);

    ASSERT_TRUE(notes.has_value());
    EXPECT_THAT(*notes, IsEmpty());
}

TEST(TranscriberTest, FTX02_SortsNotesByStartAndPitch)
{
    FakeModel model{{{.pitchIndex = 40, .from = 60, .to = 100},
                     {.pitchIndex = 35, .from = 30, .to = 100},
                     {.pitchIndex = 31, .from = 30, .to = 100}}};

    const auto notes = transcribe(oneSecond(), kRate, model, guitar(), always);

    ASSERT_TRUE(notes.has_value());
    ASSERT_EQ(notes->size(), 3U);
    EXPECT_EQ((*notes)[0].pitch, 31 + kLowestPitch);
    EXPECT_EQ((*notes)[1].pitch, 35 + kLowestPitch);
    EXPECT_EQ((*notes)[2].pitch, 40 + kLowestPitch);
}

TEST(TranscriberTest, FTX01_CancelledTranscriptionHasNoResult)
{
    FakeModel model{{}};

    EXPECT_FALSE(transcribe(oneSecond(), kRate, model, guitar(), [](double) { return false; }).has_value());
}

TEST(TranscriberTest, E12_TranscribesTheGuitarExcerptLikeBasicPitch)
{
    std::ifstream file{test::modelFile(), std::ios::binary};
    const std::vector<char> bytes{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    OnnxPitchModel model{bytes.data(), bytes.size()};
    auto reference = test::readNotes("clean");
    std::erase_if(reference, [](const test::ReferenceNote& note) { return note.note.amplitude < 0.3F; });

    const auto notes = transcribe(test::readFloats("clean-22050.f32"), kRate, model, guitar(), always);

    ASSERT_TRUE(notes.has_value());
    ASSERT_EQ(notes->size(), reference.size());
    for (const auto& note : *notes)
        EXPECT_TRUE(std::ranges::any_of(reference,
                                        [&note](const test::ReferenceNote& other)
                                        {
                                            return other.note.pitch == note.pitch &&
                                                   std::abs(other.startSeconds - note.startSeconds) < 1e-6 &&
                                                   std::abs(other.endSeconds - note.endSeconds) < 1e-6;
                                        }))
            << note.pitch << " at " << note.startSeconds;
}

TEST(TranscriberTest, FTX02_NotesDifferInEveryField)
{
    const Note note{.startSeconds = 1.0, .endSeconds = 2.0, .pitch = 40, .amplitude = 0.5F};

    EXPECT_EQ(note, note);
    EXPECT_NE(note, (Note{.startSeconds = 1.5, .endSeconds = 2.0, .pitch = 40, .amplitude = 0.5F}));
    EXPECT_NE(note, (Note{.startSeconds = 1.0, .endSeconds = 2.5, .pitch = 40, .amplitude = 0.5F}));
    EXPECT_NE(note, (Note{.startSeconds = 1.0, .endSeconds = 2.0, .pitch = 41, .amplitude = 0.5F}));
    EXPECT_NE(note, (Note{.startSeconds = 1.0, .endSeconds = 2.0, .pitch = 40, .amplitude = 0.6F}));
}

} // namespace
} // namespace snotes::transcription
