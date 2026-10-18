#include "model/InstrumentProfile.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace snotes::model
{
namespace
{

using ::testing::ElementsAre;

TEST(InstrumentProfileTest, FAU01_OffersGuitarsAndBassesInStandardTunings)
{
    std::vector<std::string> ids;
    for (const auto& profile : instrumentProfiles())
        ids.push_back(profile.id);

    EXPECT_THAT(ids, ElementsAre("guitar6-e", "guitar7-gs", "bass4-e", "bass5-gs"));
}

TEST(InstrumentProfileTest, FAU01_StringsAreMidiNotesFromTheLowestString)
{
    EXPECT_THAT(findInstrumentProfile("guitar6-e")->strings, ElementsAre(40, 45, 50, 55, 59, 64));
    EXPECT_THAT(findInstrumentProfile("bass5-gs")->strings, ElementsAre(20, 25, 30, 35, 40));
}

TEST(InstrumentProfileTest, FAU01_BassesUseTheBassClef)
{
    EXPECT_EQ(findInstrumentProfile("bass4-e")->clef, Clef::bass8vb);
    EXPECT_EQ(findInstrumentProfile("guitar7-gs")->clef, Clef::treble8vb);
    EXPECT_EQ(findInstrumentProfile("guitar7-gs")->frets, 24);
    EXPECT_TRUE(findInstrumentProfile("bass4-e")->transcribeOctaveUp);
    EXPECT_TRUE(findInstrumentProfile("bass5-gs")->transcribeOctaveUp);
    EXPECT_FALSE(findInstrumentProfile("guitar7-gs")->transcribeOctaveUp);
}

TEST(InstrumentProfileTest, FAU01_UnknownIdFindsNothing)
{
    EXPECT_FALSE(findInstrumentProfile("banjo").has_value());
}

TEST(InstrumentProfileTest, FAU01_DefaultIsTheSixStringGuitar)
{
    EXPECT_EQ(defaultInstrumentProfile().id, "guitar6-e");
}

} // namespace
} // namespace snotes::model
