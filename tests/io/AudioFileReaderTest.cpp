#include "io/AudioFileReader.h"

#include "io/Utf8Path.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>

namespace snotes::io
{
namespace
{

namespace fs = std::filesystem;
using ::testing::ElementsAre;
using ::testing::FloatNear;

class AudioFileReaderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        root = fs::temp_directory_path() / "snotes_tests" / testInfo->name();
        fs::remove_all(root);
        fs::create_directories(root);
    }

    void TearDown() override { fs::remove_all(root); }

    /// A 16-bit WAV file with two channels: left 0.5, -0.5, 0.25 and right 0, 0.25, 0.5.
    void writeStereoWav(const fs::path& file) const
    {
        const juce::File target{juce::String::fromUTF8(utf8FromPath(file).c_str())};
        juce::WavAudioFormat format;
        std::unique_ptr<juce::OutputStream> stream = target.createOutputStream();
        ASSERT_NE(stream, nullptr);
        const auto options =
            juce::AudioFormatWriterOptions{}.withSampleRate(44100.0).withNumChannels(2).withBitsPerSample(16);
        const std::unique_ptr<juce::AudioFormatWriter> writer = format.createWriterFor(stream, options);
        ASSERT_NE(writer, nullptr);
        juce::AudioBuffer<float> buffer{2, 3};
        for (const auto& [index, left, right] :
             {std::tuple{0, 0.5F, 0.0F}, {1, -0.5F, 0.25F}, {2, 0.25F, 0.5F}})
        {
            buffer.setSample(0, index, left);
            buffer.setSample(1, index, right);
        }
        ASSERT_TRUE(writer->writeFromAudioSampleBuffer(buffer, 0, 3));
    }

    fs::path root;
    AudioFileReader reader;
};

TEST_F(AudioFileReaderTest, FPJ05_ReadsAllChannelsAndTheSampleRate)
{
    const auto file = root / u8"Stimme 1 Täke DI.wav";
    writeStereoWav(file);

    const auto content = reader.read(file);

    ASSERT_TRUE(content.has_value());
    EXPECT_DOUBLE_EQ(content->sampleRate, 44100.0);
    ASSERT_EQ(content->channels.size(), 2U);
    constexpr float kTolerance = 1.0F / 16384.0F;
    EXPECT_THAT(
        content->channels[0],
        ElementsAre(FloatNear(0.5F, kTolerance), FloatNear(-0.5F, kTolerance), FloatNear(0.25F, kTolerance)));
    EXPECT_THAT(
        content->channels[1],
        ElementsAre(FloatNear(0.0F, kTolerance), FloatNear(0.25F, kTolerance), FloatNear(0.5F, kTolerance)));
}

TEST_F(AudioFileReaderTest, FPJ05_DoesNotReadAMissingFile)
{
    EXPECT_FALSE(reader.read(root / "missing.wav").has_value());
}

TEST_F(AudioFileReaderTest, FPJ05_DoesNotReadAFileThatIsNoAudio)
{
    const auto file = root / "notes.wav";
    std::ofstream{file} << "no audio";

    EXPECT_FALSE(reader.read(file).has_value());
}

TEST_F(AudioFileReaderTest, FPJ05_DoesNotReadAnAudioFileWithoutSamples)
{
    const auto file = root / "empty.wav";
    {
        const juce::File target{juce::String::fromUTF8(utf8FromPath(file).c_str())};
        juce::WavAudioFormat format;
        std::unique_ptr<juce::OutputStream> stream = target.createOutputStream();
        const auto options =
            juce::AudioFormatWriterOptions{}.withSampleRate(48000.0).withNumChannels(1).withBitsPerSample(24);
        const std::unique_ptr<juce::AudioFormatWriter> writer = format.createWriterFor(stream, options);
        ASSERT_NE(writer, nullptr);
    }

    EXPECT_FALSE(reader.read(file).has_value());
}

} // namespace
} // namespace snotes::io
