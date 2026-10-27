#include "engine/NoteSynth.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace snotes::engine
{
namespace
{

constexpr double kRate = 1000.0;
/// 50 ms release at 1 kHz.
constexpr std::int64_t kRelease = 50;

/// Renders numSamples samples from position on into two outputs, the second one inactive.
std::vector<float> render(const SynthNotes& notes, std::int64_t position, int numSamples, float gain = 1.0F)
{
    std::vector<float> samples(static_cast<std::size_t>(numSamples));
    std::array<float*, 2> outputs{samples.data(), nullptr};
    NoteSynth::render(notes, kRate, position, gain, outputs.data(), 2, numSamples);
    return samples;
}

float peak(const std::vector<float>& samples)
{
    float result = 0.0F;
    for (const float value : samples)
        result = std::max(result, std::abs(value));
    return result;
}

TEST(NoteSynthTest, FNA05_ANoteSoundsFromItsStartUntilItsRelease)
{
    const SynthNote note{.start = 100, .end = 300, .frequency = 110.0};

    EXPECT_EQ(NoteSynth::sampleAt(note, 99, kRate), 0.0F);
    EXPECT_EQ(NoteSynth::sampleAt(note, 100, kRate), 0.0F);
    EXPECT_NE(NoteSynth::sampleAt(note, 103, kRate), 0.0F);
    EXPECT_NE(NoteSynth::sampleAt(note, 300 + kRelease - 1, kRate), 0.0F);
    EXPECT_EQ(NoteSynth::sampleAt(note, 300 + kRelease, kRate), 0.0F);
}

TEST(NoteSynthTest, FNA05_ANoteRisesQuicklyAndDiesAway)
{
    constexpr double kHighRate = 48000.0;
    const SynthNote note{.start = 0, .end = 96000, .frequency = 110.0};
    float early = 0.0F;
    float late = 0.0F;
    for (std::int64_t sample = 240; sample < 4800; ++sample)
        early = std::max(early, std::abs(NoteSynth::sampleAt(note, sample, kHighRate)));
    for (std::int64_t sample = 72000; sample < 76800; ++sample)
        late = std::max(late, std::abs(NoteSynth::sampleAt(note, sample, kHighRate)));

    EXPECT_GT(early, 0.2F);
    EXPECT_LE(early, NoteSynth::kLevel);
    EXPECT_LT(late, early / 4.0F);
}

TEST(NoteSynthTest, FNA05_RendersTheSoundingNotesIntoAllActiveOutputs)
{
    SynthNotes notes{.notes = {{.start = 0, .end = 20, .frequency = 110.0},
                               {.start = 40, .end = 400, .frequency = 220.0},
                               {.start = 500, .end = 520, .frequency = 330.0}},
                     .longest = 360};

    const auto block = render(notes, 300, 100, 0.5F);

    for (std::size_t index = 0; index < block.size(); ++index)
    {
        const auto sample = 300 + static_cast<std::int64_t>(index);
        const auto expected = 0.5F * NoteSynth::sampleAt(notes.notes[1], sample, kRate);
        EXPECT_FLOAT_EQ(block[index], expected) << sample;
    }
    EXPECT_GT(peak(block), 0.0F);
    // Silence between the notes and after them.
    EXPECT_EQ(peak(render(notes, 460, 40)), 0.0F);
    EXPECT_EQ(peak(render(notes, 600, 100)), 0.0F);
    EXPECT_GT(peak(render(notes, 450, 100)), 0.0F);
    EXPECT_EQ(peak(render({}, 0, 100)), 0.0F);
}

TEST(NoteSynthTest, FNA05_NotesDifferInEveryField)
{
    const SynthNote note{.start = 1, .end = 2, .frequency = 110.0};

    EXPECT_EQ(note, note);
    EXPECT_NE(note, (SynthNote{.start = 0, .end = 2, .frequency = 110.0}));
    EXPECT_NE(note, (SynthNote{.start = 1, .end = 3, .frequency = 110.0}));
    EXPECT_NE(note, (SynthNote{.start = 1, .end = 2, .frequency = 220.0}));
}

} // namespace
} // namespace snotes::engine
