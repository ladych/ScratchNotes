#include "rhythm/NoteEdit.h"

#include <gtest/gtest.h>

#include <vector>

namespace snotes::rhythm
{
namespace
{

constexpr int kNone = NoteEdit::kNone;

QuantizedEvent event(double seconds, std::vector<int> pitches)
{
    QuantizedEvent result;
    result.position = Fraction{static_cast<std::int64_t>(seconds * 2.0)};
    result.duration = Fraction{1};
    result.seconds = seconds;
    result.pitches = std::move(pitches);
    result.graces = {{40}};
    return result;
}

const std::vector<QuantizedEvent> kEvents{event(0.5, {40, 47}), event(1.0, {45})};

TEST(NoteEditTest, FKO04_WithoutEditsTheEventsStay)
{
    const auto edited = applyNoteEdits(kEvents, {});
    EXPECT_EQ(edited.events, kEvents);
    EXPECT_EQ(edited.strings, (std::vector<std::vector<int>>{{}, {}}));
}

TEST(NoteEditTest, FKO04_ADeletedNoteGoesAndAnEmptyEventToo)
{
    const auto edited = applyNoteEdits(
        kEvents,
        {{.seconds = 0.50005, .pitch = 47, .deleted = true}, {.seconds = 1.0, .pitch = 45, .deleted = true}});
    ASSERT_EQ(edited.events.size(), 1U);
    EXPECT_EQ(edited.events[0].pitches, (std::vector<int>{40}));
    EXPECT_EQ(edited.events[0].graces, kEvents[0].graces);
}

TEST(NoteEditTest, FKO04_ATransposedNoteKeepsThePitchesAscending)
{
    const auto edited = applyNoteEdits(kEvents, {{.seconds = 0.5, .pitch = 40, .toPitch = 52}});
    EXPECT_EQ(edited.events[0].pitches, (std::vector<int>{47, 52}));
    EXPECT_EQ(edited.events[1], kEvents[1]);
}

TEST(NoteEditTest, FKO04_TheDurationOfAnEventChanges)
{
    const auto edited = applyNoteEdits(kEvents,
                                       {{.seconds = 1.0, .pitch = kNone, .durationTicks = 6},
                                        {.seconds = 0.5, .pitch = kNone, .durationTicks = 0}});
    EXPECT_EQ(edited.events[0].duration, Fraction{1});
    EXPECT_EQ(edited.events[1].duration, (Fraction{1, 2}));
}

TEST(NoteEditTest, FTB03_FixedStringsFollowTheirPitches)
{
    const auto edited = applyNoteEdits(kEvents, {{.seconds = 0.5, .pitch = 40, .toPitch = 50, .string = 3}});
    EXPECT_EQ(edited.events[0].pitches, (std::vector<int>{47, 50}));
    EXPECT_EQ(edited.strings[0], (std::vector<int>{kNone, 3}));
    EXPECT_TRUE(edited.strings[1].empty());

    // A string alone keeps the pitch; two fixed strings both count.
    const auto both = applyNoteEdits(
        kEvents, {{.seconds = 0.5, .pitch = 40, .string = 0}, {.seconds = 0.5, .pitch = 47, .string = 1}});
    EXPECT_EQ(both.events[0].pitches, kEvents[0].pitches);
    EXPECT_EQ(both.strings[0], (std::vector<int>{0, 1}));
}

TEST(NoteEditTest, FKO04_EditsCompareAllFields)
{
    const NoteEdit edit{
        .seconds = 0.5, .pitch = 40, .deleted = false, .toPitch = 41, .string = 2, .durationTicks = 6};
    auto other = edit;
    EXPECT_EQ(other, edit);
    other.seconds = 0.6;
    EXPECT_NE(other, edit);
    other = edit;
    other.pitch = 41;
    EXPECT_NE(other, edit);
    other = edit;
    other.toPitch = 42;
    EXPECT_NE(other, edit);
    other = edit;
    other.string = 3;
    EXPECT_NE(other, edit);
    other = edit;
    other.durationTicks = 3;
    EXPECT_NE(other, edit);
    EXPECT_NE((EditedEvents{.events = {}, .strings = {{}}}), (EditedEvents{}));
    EXPECT_NE((EditedEvents{.events = {QuantizedEvent{}}, .strings = {}}), (EditedEvents{}));
    EXPECT_EQ(applyNoteEdits({}, {}), (EditedEvents{}));
}

TEST(NoteEditTest, FKO05_EditsFindTheirNotesBySecondsAndPitch)
{
    const NoteEdit edit{.seconds = 0.5, .pitch = 40};
    EXPECT_TRUE(edit.sameNote({.seconds = 0.50009, .pitch = 40, .deleted = true}));
    EXPECT_FALSE(edit.sameNote({.seconds = 0.5002, .pitch = 40}));
    EXPECT_FALSE(edit.sameNote({.seconds = 0.5, .pitch = 41}));
    // Edits of notes the events do not have change nothing.
    EXPECT_EQ(applyNoteEdits(kEvents, {{.seconds = 2.0, .pitch = 40, .deleted = true}}).events, kEvents);
    EXPECT_NE(edit, (NoteEdit{.seconds = 0.5, .pitch = 40, .deleted = true}));
}

} // namespace
} // namespace snotes::rhythm
