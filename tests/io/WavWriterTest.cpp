#include "io/WavWriter.h"

#include "io/FakeBinaryFile.h"

#include <gtest/gtest.h>

#include <array>
#include <memory>
#include <string>

namespace snotes::io
{
namespace
{

class WavWriterTest : public ::testing::Test
{
protected:
    [[nodiscard]] std::string text(std::size_t offset) const
    {
        std::string chunkId(4, ' ');
        for (std::size_t index = 0; index < 4; ++index)
            chunkId[index] = static_cast<char>(content->bytes.at(offset + index));
        return chunkId;
    }

    std::shared_ptr<FakeFileContent> content = std::make_shared<FakeFileContent>();
    std::unique_ptr<WavWriter> writer;

    void open(int failAtCall = 0)
    {
        content->failAtCall = failAtCall;
        writer = std::make_unique<WavWriter>(std::make_unique<FakeBinaryFile>(content), 48000.0);
    }
};

TEST_F(WavWriterTest, FAU02_HeaderDescribesMono32BitFloat)
{
    open();

    ASSERT_EQ(content->bytes.size(), WavWriter::kHeaderSize);
    EXPECT_EQ(text(0), "RIFF");
    EXPECT_EQ(text(8), "WAVE");
    EXPECT_EQ(text(12), "fmt ");
    EXPECT_EQ(content->uint32At(16), 18U);
    EXPECT_EQ(content->uint32At(20) & 0xFFFFU, 3U);      // IEEE float
    EXPECT_EQ(content->uint32At(20) >> 16U, 1U);         // mono
    EXPECT_EQ(content->uint32At(24), 48000U);            // sample rate
    EXPECT_EQ(content->uint32At(28), 48000U * 4U);       // bytes per second
    EXPECT_EQ(content->uint32At(32), 4U | (32U << 16U)); // block align, bits per sample
    EXPECT_EQ(text(38), "fact");
    EXPECT_EQ(text(50), "data");
    EXPECT_EQ(content->uint32At(54), 0U);
    EXPECT_TRUE(writer->ok());
}

TEST_F(WavWriterTest, FAU02_AppendsSamplesAndUpdatesTheSizes)
{
    open();
    const std::array<float, 3> samples{0.5F, -0.25F, 1.0F};

    EXPECT_TRUE(writer->append(samples));
    EXPECT_TRUE(writer->updateHeader());

    EXPECT_EQ(writer->numSamples(), 3U);
    EXPECT_EQ(content->uint32At(4), WavWriter::kHeaderSize - 8 + 12);
    EXPECT_EQ(content->uint32At(46), 3U);
    EXPECT_EQ(content->uint32At(54), 12U);
    EXPECT_FLOAT_EQ(content->floatAt(WavWriter::kHeaderSize + 4), -0.25F);
    EXPECT_EQ(content->flushes, 1);
}

TEST_F(WavWriterTest, FAU02_FailedHeaderStopsTheWriter)
{
    open(1);
    const std::array<float, 1> samples{0.5F};

    EXPECT_FALSE(writer->ok());
    EXPECT_FALSE(writer->append(samples));
    EXPECT_FALSE(writer->updateHeader());
}

TEST_F(WavWriterTest, FAU02_FailedAppendIsReported)
{
    open(2);
    const std::array<float, 1> samples{0.5F};

    EXPECT_FALSE(writer->append(samples));
    EXPECT_FALSE(writer->ok());
}

TEST_F(WavWriterTest, FAU02_EveryFailingStepOfTheHeaderUpdateIsReported)
{
    // Calls: 1 header, 2-4 the three sizes, 5 flush.
    for (int failing = 2; failing <= 5; ++failing)
    {
        content = std::make_shared<FakeFileContent>();
        open(failing);

        EXPECT_FALSE(writer->updateHeader()) << failing;
        EXPECT_FALSE(writer->ok()) << failing;
    }
}

} // namespace
} // namespace snotes::io
