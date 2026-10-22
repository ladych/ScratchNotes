#include "transcription/LevelGate.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <initializer_list>
#include <vector>

namespace snotes::transcription
{
namespace
{

using ::testing::ElementsAre;

constexpr double kRate = 22050.0;

Note noteAt(double seconds, float amplitude = 0.8F)
{
    return {.startSeconds = seconds, .endSeconds = seconds + 0.5, .pitch = 40, .amplitude = amplitude};
}

/// One second of loud signal followed by one second at the given level.
std::vector<float> loudThen(float level)
{
    std::vector<float> audio(static_cast<std::size_t>(2 * kRate), level);
    std::fill(audio.begin(), audio.begin() + static_cast<std::ptrdiff_t>(kRate), 0.5F);
    return audio;
}

TEST(LevelGateTest, FTX02_DropsNotesInNearSilence)
{
    const std::vector<Note> notes{noteAt(0.2), noteAt(1.5)};

    EXPECT_THAT(applyLevelGate(notes, loudThen(0.5F * 0.001F), GateSettings{}), ElementsAre(noteAt(0.2)));
    EXPECT_EQ(applyLevelGate(notes, loudThen(0.5F * 0.01F), GateSettings{}).size(), 2U);
}

TEST(LevelGateTest, FTX02_DropsUncertainNotes)
{
    const std::vector<Note> notes{noteAt(0.2, 0.29F), noteAt(0.4, 0.3F)};

    EXPECT_THAT(applyLevelGate(notes, loudThen(0.5F), GateSettings{}), ElementsAre(noteAt(0.4, 0.3F)));
}

TEST(LevelGateTest, FTX02_NotesAtTheEndUseTheLastLevel)
{
    const std::vector<Note> notes{noteAt(2.5)};

    EXPECT_EQ(applyLevelGate(notes, loudThen(0.5F), GateSettings{}).size(), 1U);
    EXPECT_TRUE(applyLevelGate(notes, loudThen(0.0F), GateSettings{}).empty());
}

/// One second each of the given levels, one after the other.
std::vector<float> sections(std::initializer_list<float> levels)
{
    std::vector<float> audio;
    for (const auto level : levels)
        audio.insert(audio.end(), static_cast<std::size_t>(kRate), level);
    return audio;
}

TEST(LevelGateTest, FTX02_DropsQuietNotesBeforeThePlayingStarts)
{
    // 40 dB below the loudest moment: through the gate, but noise of the lead-in before the first loud note.
    const auto audio = sections({0.5F * 0.01F, 0.5F, 0.5F * 0.01F});
    const std::vector<Note> notes{noteAt(0.5), noteAt(1.5), noteAt(2.5)};

    EXPECT_THAT(applyLevelGate(notes, audio, GateSettings{}), ElementsAre(noteAt(1.5), noteAt(2.5)));
    EXPECT_EQ(applyLevelGate(notes, audio, GateSettings{.leadInRangeDb = 45.0}).size(), 3U);
}

} // namespace
} // namespace snotes::transcription
