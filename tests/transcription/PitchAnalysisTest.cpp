#include "transcription/PitchAnalysis.h"

#include "transcription/OnnxPitchModel.h"
#include "transcription/ReferenceData.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>

namespace snotes::transcription
{
namespace
{

using namespace basicpitch;

/// Writes the number of the window into every frame value, and the first sample into every onset value.
class FakeModel : public IPitchModel
{
public:
    void run(std::span<const float> window, std::span<float> frames, std::span<float> onsets) override
    {
        windows.emplace_back(window.begin(), window.end());
        std::ranges::fill(frames, static_cast<float>(windows.size() - 1));
        std::ranges::fill(onsets, window.front());
    }

    std::vector<std::vector<float>> windows;
};

bool always(double /*done*/)
{
    return true;
}

TEST(PitchAnalysisTest, FTX02_PadsAndCutsTheAudioIntoOverlappingWindows)
{
    std::vector<float> audio(kWindowHop + 100);
    for (std::size_t index = 0; index < audio.size(); ++index)
        audio[index] = static_cast<float>(index);
    FakeModel model;

    const auto result = analysePitches(audio, model, always);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(model.windows.size(), 2U);
    EXPECT_EQ(model.windows[0][(kOverlapSamples / 2) - 1], 0.0F);
    EXPECT_EQ(model.windows[0][kOverlapSamples / 2], 0.0F);
    EXPECT_EQ(model.windows[0][(kOverlapSamples / 2) + 1], 1.0F);
    EXPECT_EQ(model.windows[1][0], static_cast<float>(kWindowHop - (kOverlapSamples / 2)));
    EXPECT_EQ(model.windows[1].back(), 0.0F);
}

TEST(PitchAnalysisTest, FTX02_JoinsTheWindowsWithoutOverlapAndTrimsToTheAudio)
{
    const std::vector<float> audio(2 * kWindowHop);
    FakeModel model;

    const auto result = analysePitches(audio, model, always);

    ASSERT_TRUE(result.has_value());
    const int kept = kFramesPerWindow - kOverlapFrames;
    EXPECT_EQ(result->numFrames, static_cast<int>((audio.size() * kFramesPerSecond) / 22050));
    EXPECT_EQ(result->frames.size(), Activations::index(result->numFrames, 0));
    EXPECT_EQ(result->onsets.size(), result->frames.size());
    EXPECT_EQ(result->frame(kept - 1, 0), 0.0F);
    EXPECT_EQ(result->frame(kept, 0), 1.0F);
    EXPECT_EQ(result->frame(result->numFrames - 1, kNumPitches - 1), 1.0F);
}

TEST(PitchAnalysisTest, FTX01_ReportsProgressAndStopsWhenCancelled)
{
    const std::vector<float> audio(3 * kWindowHop);
    FakeModel model;
    std::vector<double> reported;

    const auto result = analysePitches(audio,
                                       model,
                                       [&reported](double done)
                                       {
                                           reported.push_back(done);
                                           return reported.size() < 2;
                                       });

    EXPECT_FALSE(result.has_value());
    EXPECT_THAT(reported, ::testing::ElementsAre(0.0, 0.25));
    EXPECT_EQ(model.windows.size(), 1U);
}

TEST(PitchAnalysisTest, FTX02_EmptyAudioHasNoFrames)
{
    FakeModel model;

    const auto result = analysePitches({}, model, always);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->numFrames, 0);
    EXPECT_TRUE(result->frames.empty());
}

std::vector<char> modelBytes()
{
    std::ifstream file{test::modelFile(), std::ios::binary};
    return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

TEST(PitchAnalysisTest, E12_ActivationsOfTheOnnxModelEqualThePythonReference)
{
    const auto bytes = modelBytes();
    OnnxPitchModel model{bytes.data(), bytes.size()};
    const auto audio = test::readFloats("clean-22050.f32");
    const auto expected = test::readActivations("clean");

    const auto result = analysePitches(audio, model, always);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->numFrames, expected.numFrames);
    float largest = 0.0F;
    for (std::size_t cell = 0; cell < expected.frames.size(); ++cell)
    {
        largest = std::max(largest, std::abs(result->frames[cell] - expected.frames[cell]));
        largest = std::max(largest, std::abs(result->onsets[cell] - expected.onsets[cell]));
    }
    EXPECT_LT(largest, 1e-4F);
}

TEST(PitchAnalysisTest, FTX02_InvalidModelIsReported)
{
    const std::vector<char> garbage(64, 'x');

    EXPECT_THROW(OnnxPitchModel(garbage.data(), garbage.size()), std::runtime_error);
}

} // namespace
} // namespace snotes::transcription
