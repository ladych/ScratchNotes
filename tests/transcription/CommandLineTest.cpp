#include "transcription/CommandLine.h"
#include "transcription/NoteCsv.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace snotes::transcription
{
namespace
{

using ::testing::ElementsAre;
using ::testing::HasSubstr;

TEST(CommandLineTest, FTX02_ReadsProfileFolderAndRecordings)
{
    const auto parsed = parseCommandLine({"--profile", "bass5-gs", "a.wav", "--out", "results", "b.wav"});

    ASSERT_TRUE(parsed.options.has_value());
    EXPECT_EQ(parsed.options->profileId, "bass5-gs");
    EXPECT_EQ(parsed.options->outputFolder, "results");
    EXPECT_THAT(parsed.options->recordings, ElementsAre("a.wav", "b.wav"));
}

TEST(CommandLineTest, FTX02_DefaultsToTheSixStringGuitarNextToTheRecording)
{
    const auto parsed = parseCommandLine({"a.wav"});

    ASSERT_TRUE(parsed.options.has_value());
    EXPECT_EQ(parsed.options->profileId, "guitar6-e");
    EXPECT_TRUE(parsed.options->outputFolder.empty());
}

TEST(CommandLineTest, FTX02_WrongArgumentsExplainWhy)
{
    EXPECT_THAT(parseCommandLine({}).message, HasSubstr("No recording given"));
    EXPECT_THAT(parseCommandLine({"a.wav", "--profile"}).message, HasSubstr("Missing value after --profile"));
    EXPECT_THAT(parseCommandLine({"--out"}).message, HasSubstr("Missing value after --out"));
    EXPECT_THAT(parseCommandLine({"--profile", "banjo", "a.wav"}).message,
                HasSubstr("Unknown profile banjo"));
    EXPECT_THAT(parseCommandLine({"--fast", "a.wav"}).message, HasSubstr("Unknown option --fast"));
    EXPECT_FALSE(parseCommandLine({"--fast", "a.wav"}).options.has_value());
}

TEST(CommandLineTest, FTX02_HelpListsTheProfiles)
{
    for (const char* help : {"--help", "-h"})
    {
        const auto parsed = parseCommandLine({help});

        EXPECT_FALSE(parsed.options.has_value());
        EXPECT_THAT(parsed.message, HasSubstr("Usage: scratchnotes-transcribe"));
        EXPECT_THAT(parsed.message, HasSubstr("  guitar6-e (default)\n"));
        EXPECT_THAT(parsed.message, HasSubstr("  bass5-gs\n"));
    }
}

TEST(NoteCsvTest, FTX02_WritesOneLinePerNoteWithDecimalPoints)
{
    const std::vector<Note> notes{
        {.startSeconds = 0.5, .endSeconds = 1.25, .pitch = 40, .amplitude = 0.8F},
        {.startSeconds = 2.0, .endSeconds = 2.123456, .pitch = 52, .amplitude = 0.4567F}};

    EXPECT_EQ(notesToCsv(notes),
              "start,end,pitch,amplitude\n0.5000,1.2500,40,0.800\n2.0000,2.1235,52,0.457\n");
    EXPECT_EQ(notesToCsv({}), "start,end,pitch,amplitude\n");
}

} // namespace
} // namespace snotes::transcription
