#include "io/TakeFileWriter.h"

#include "io/FakeBinaryFile.h"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <vector>

namespace snotes::io
{
namespace
{

class TakeFileWriterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        std::vector<std::unique_ptr<WavWriter>> writers;
        for (auto& content : contents)
            writers.push_back(
                std::make_unique<WavWriter>(std::make_unique<FakeBinaryFile>(content), 48000.0));
        writer = std::make_unique<TakeFileWriter>(session, std::move(writers), 4);
    }

    void record(float value, int numSamples)
    {
        std::vector<float> left(static_cast<std::size_t>(numSamples), value);
        std::vector<float> right(static_cast<std::size_t>(numSamples), -value);
        const std::array<const float*, 2> channels{left.data(), right.data()};
        session.record(channels.data(), 2, numSamples);
    }

    std::array<std::shared_ptr<FakeFileContent>, 2> contents{std::make_shared<FakeFileContent>(),
                                                             std::make_shared<FakeFileContent>()};
    engine::RecordingSession session{{0, 1}, 64};
    std::unique_ptr<TakeFileWriter> writer;
};

TEST_F(TakeFileWriterTest, FAU02_WritesEachChannelIntoItsFile)
{
    record(0.5F, 3);

    EXPECT_TRUE(writer->finish());

    EXPECT_EQ(contents[0]->bytes.size(), WavWriter::kHeaderSize + 12);
    EXPECT_FLOAT_EQ(contents[0]->floatAt(WavWriter::kHeaderSize), 0.5F);
    EXPECT_FLOAT_EQ(contents[1]->floatAt(WavWriter::kHeaderSize), -0.5F);
    EXPECT_EQ(contents[1]->uint32At(54), 12U);
}

TEST_F(TakeFileWriterTest, FAU02_HeadersAreUpdatedWhileRecording)
{
    record(0.5F, 2);
    writer->pump();
    EXPECT_EQ(contents[0]->uint32At(54), 0U);

    record(0.5F, 2);
    writer->pump();
    EXPECT_EQ(contents[0]->uint32At(54), 16U);
    EXPECT_EQ(contents[1]->flushes, 1);
}

TEST_F(TakeFileWriterTest, FAU02_ReadsLargeBlocksInChunks)
{
    engine::RecordingSession large{{0}, 20000};
    auto content = std::make_shared<FakeFileContent>();
    std::vector<std::unique_ptr<WavWriter>> writers;
    writers.push_back(std::make_unique<WavWriter>(std::make_unique<FakeBinaryFile>(content), 48000.0));
    TakeFileWriter largeWriter{large, std::move(writers)};
    std::vector<float> samples(10000, 0.25F);
    const std::array<const float*, 1> channels{samples.data()};
    large.record(channels.data(), 1, 10000);

    EXPECT_TRUE(largeWriter.finish());

    EXPECT_EQ(content->uint32At(46), 10000U);
}

TEST_F(TakeFileWriterTest, FAU02_FailedAppendIsReported)
{
    contents[1]->failAtCall = 2;
    record(0.5F, 2);

    writer->pump();

    EXPECT_TRUE(writer->failed());
    EXPECT_FALSE(writer->finish());
}

TEST_F(TakeFileWriterTest, FAU02_FailedHeaderUpdateIsReported)
{
    contents[0]->failAtCall = 3;

    EXPECT_FALSE(writer->finish());
}

} // namespace
} // namespace snotes::io
