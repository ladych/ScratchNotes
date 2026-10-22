#include "notation/MusicXml.h"

#include "notation/ScoreRenderer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <juce_core/juce_core.h>

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace snotes::notation
{
namespace
{

using ::testing::HasSubstr;

const tab::Fretboard kGuitar{.strings = {40, 45, 50, 55, 59, 64}, .frets = 24};

constexpr NoteValue kQuarter{.ticks = 12, .type = "quarter", .dots = 0, .triplet = false};
constexpr NoteValue kEighth{.ticks = 6, .type = "eighth", .dots = 0, .triplet = false};
constexpr NoteValue kTripletEighth{.ticks = 4, .type = "eighth", .dots = 0, .triplet = true};
constexpr NoteValue kThirtySecondTriplet{.ticks = 1, .type = "32nd", .dots = 0, .triplet = true};

TEST(MusicXmlTest, FNA01_NotesWithinABeatTakeTheLongestValuesFirst)
{
    EXPECT_EQ(noteValues(0, 9), (std::vector<NoteValue>{{.ticks = 9, .type = "eighth", .dots = 1}}));
    EXPECT_EQ(noteValues(0, 8), (std::vector<NoteValue>{{.ticks = 8, .type = "quarter", .triplet = true}}));
    EXPECT_EQ(noteValues(6, 6), std::vector<NoteValue>{kEighth});
    EXPECT_EQ(noteValues(4, 5), (std::vector<NoteValue>{kTripletEighth, kThirtySecondTriplet}));
    EXPECT_EQ(noteValues(9, 3), (std::vector<NoteValue>{{.ticks = 3, .type = "16th"}}));
    EXPECT_EQ(noteValues(10, 2), (std::vector<NoteValue>{{.ticks = 2, .type = "16th", .triplet = true}}));
}

TEST(MusicXmlTest, FNA01_WholeBeatsBecomeQuarterHalfAndWholeNotes)
{
    EXPECT_EQ(noteValues(0, 12), std::vector<NoteValue>{kQuarter});
    EXPECT_EQ(noteValues(12, 24), (std::vector<NoteValue>{{.ticks = 24, .type = "half"}}));
    EXPECT_EQ(noteValues(0, 36), (std::vector<NoteValue>{{.ticks = 36, .type = "half", .dots = 1}}));
    EXPECT_EQ(noteValues(0, 60), (std::vector<NoteValue>{{.ticks = 48, .type = "whole"}, kQuarter}));
}

TEST(MusicXmlTest, FNA01_NotesAcrossBeatsAreSplitAtTheBeats)
{
    EXPECT_EQ(noteValues(6, 12), (std::vector<NoteValue>{kEighth, kEighth}));
    EXPECT_EQ(noteValues(6, 24), (std::vector<NoteValue>{kEighth, kQuarter, kEighth}));
}

TEST(MusicXmlTest, FNA01_NoteValuesDifferInEveryPart)
{
    EXPECT_NE(kQuarter, (NoteValue{.ticks = 11, .type = "quarter"}));
    EXPECT_NE(kQuarter, (NoteValue{.ticks = 12, .type = "half"}));
    EXPECT_NE(kQuarter, (NoteValue{.ticks = 12, .type = "quarter", .dots = 1}));
    EXPECT_NE(kQuarter, (NoteValue{.ticks = 12, .type = "quarter", .triplet = true}));
}

ScoreEvent note(std::int64_t start, std::int64_t duration, std::vector<int> pitches, tab::Shape shape)
{
    return {.startTick = start,
            .durationTicks = duration,
            .chord = {.pitches = std::move(pitches), .shape = std::move(shape)},
            .graces = {}};
}

ScoreEvent openLowE(std::int64_t start, std::int64_t duration)
{
    return note(start, duration, {40}, {{.string = 0, .fret = 0}});
}

Score guitarScore(std::vector<ScoreEvent> events, std::int64_t pickup = 0, int bars = 1)
{
    return {.beatsPerBar = 4,
            .pickupTicks = pickup,
            .bars = bars,
            .voices = {{.name = "Gitarre",
                        .clef = model::Clef::treble8vb,
                        .fretboard = kGuitar,
                        .events = std::move(events)}}};
}

std::unique_ptr<juce::XmlElement> parse(const Score& score)
{
    auto xml = juce::parseXML(juce::String::fromUTF8(toMusicXml(score).c_str()));
    EXPECT_NE(xml, nullptr);
    return xml;
}

juce::XmlElement& measure(juce::XmlElement& document, int part, int index)
{
    auto* element = document.getChildByName("part");
    for (int skip = 0; skip < part; ++skip)
        element = element->getNextElementWithTagName("part");
    return *element->getChildElement(index);
}

/// The notes of a staff in a measure, grace notes included.
std::vector<juce::XmlElement*> notesOn(juce::XmlElement& measure, int staff)
{
    std::vector<juce::XmlElement*> notes;
    for (auto* element : measure.getChildWithTagNameIterator("note"))
        if (element->getChildElementAllSubText("staff", {}) == juce::String{staff})
            notes.push_back(element);
    return notes;
}

juce::String text(const juce::XmlElement* element, const char* path)
{
    const auto parts = juce::StringArray::fromTokens(path, "/", "");
    for (const auto& part : parts)
    {
        if (element == nullptr)
            return {};
        element = element->getChildByName(part);
    }
    return element == nullptr ? juce::String{} : element->getAllSubText();
}

TEST(MusicXmlTest, FNA01_APartHasAStaffOfNotesAnOctaveDownAndOneOfTablatureInItsTuning)
{
    auto xml = parse(guitarScore({openLowE(0, 48)}));
    ASSERT_NE(xml, nullptr);

    EXPECT_EQ(xml->getTagName(), "score-partwise");
    EXPECT_EQ(text(xml.get(), "part-list/score-part/part-name"), "Gitarre");
    auto& bar = measure(*xml, 0, 0);
    EXPECT_EQ(bar.getStringAttribute("number"), "1");
    const auto* attributes = bar.getChildByName("attributes");
    EXPECT_EQ(text(attributes, "divisions"), "12");
    EXPECT_EQ(text(attributes, "time/beats"), "4");
    EXPECT_EQ(text(attributes, "staves"), "2");
    const auto* clef = attributes->getChildByName("clef");
    EXPECT_EQ(text(clef, "sign"), "G");
    EXPECT_EQ(text(clef, "clef-octave-change"), "-1");
    EXPECT_EQ(text(clef->getNextElementWithTagName("clef"), "sign"), "TAB");
    const auto* details = attributes->getChildByName("staff-details");
    EXPECT_EQ(text(details, "staff-lines"), "6");
    EXPECT_EQ(text(details, "staff-tuning/tuning-step"), "E");
    EXPECT_EQ(text(details, "staff-tuning/tuning-octave"), "2");
}

TEST(MusicXmlTest, FNA01_BassStaffAndTuning)
{
    auto score = guitarScore({});
    score.voices.front().clef = model::Clef::bass8vb;
    score.voices.front().fretboard = {.strings = {32, 37, 42}, .frets = 20};
    auto xml = parse(score);
    ASSERT_NE(xml, nullptr);

    const auto* attributes = measure(*xml, 0, 0).getChildByName("attributes");
    EXPECT_EQ(text(attributes, "clef/sign"), "F");
    EXPECT_EQ(text(attributes, "clef/line"), "4");
    EXPECT_EQ(text(attributes, "staff-details/staff-lines"), "3");
    EXPECT_EQ(text(attributes, "staff-details/staff-tuning/tuning-step"), "G");
    EXPECT_EQ(text(attributes, "staff-details/staff-tuning/tuning-alter"), "1");
    EXPECT_EQ(text(attributes, "staff-details/staff-tuning/tuning-octave"), "1");
}

TEST(MusicXmlTest, FNA02_TheUpbeatIsAnImplicitBarZeroAndNotesAreTiedAcrossBars)
{
    // A beat of upbeat, then a note from the downbeat over the bar line.
    auto xml = parse(guitarScore({openLowE(0, 12), openLowE(12, 60)}, 12, 2));
    ASSERT_NE(xml, nullptr);

    auto& upbeat = measure(*xml, 0, 0);
    EXPECT_EQ(upbeat.getStringAttribute("number"), "0");
    EXPECT_EQ(upbeat.getStringAttribute("implicit"), "yes");
    EXPECT_EQ(text(upbeat.getChildByName("backup"), "duration"), "12");
    auto& first = measure(*xml, 0, 1);
    EXPECT_EQ(first.getStringAttribute("number"), "1");
    EXPECT_EQ(first.getChildByName("attributes"), nullptr);
    const auto whole = notesOn(first, 1);
    ASSERT_EQ(whole.size(), 1U);
    EXPECT_EQ(text(whole[0], "type"), "whole");
    EXPECT_EQ(whole[0]->getChildByName("tie")->getStringAttribute("type"), "start");
    auto& second = measure(*xml, 0, 2);
    const auto end = notesOn(second, 1);
    ASSERT_EQ(end.size(), 2U);
    EXPECT_EQ(end[0]->getChildByName("tie")->getStringAttribute("type"), "stop");
    EXPECT_EQ(text(end[0], "notations/tied"), "");
    EXPECT_NE(end[0]->getChildByName("notations")->getChildByName("tied"), nullptr);
    EXPECT_NE(end[1]->getChildByName("rest"), nullptr);
    EXPECT_EQ(text(end[1], "type"), "half");
    EXPECT_NE(end[1]->getChildByName("dot"), nullptr);
}

TEST(MusicXmlTest, FNA02_EmptyBarsHaveABarRestAndTheTablatureHidesItsRests)
{
    auto xml = parse(guitarScore({}, 24, 1));
    ASSERT_NE(xml, nullptr);

    const auto upbeat = notesOn(measure(*xml, 0, 0), 1);
    ASSERT_EQ(upbeat.size(), 1U);
    EXPECT_EQ(text(upbeat[0], "type"), "half");
    const auto bar = notesOn(measure(*xml, 0, 1), 1);
    ASSERT_EQ(bar.size(), 1U);
    EXPECT_EQ(bar[0]->getChildByName("rest")->getStringAttribute("measure"), "yes");
    EXPECT_EQ(text(bar[0], "duration"), "48");
    EXPECT_EQ(bar[0]->getChildByName("type"), nullptr);
    EXPECT_FALSE(bar[0]->hasAttribute("print-object"));
    const auto tablature = notesOn(measure(*xml, 0, 1), 2);
    ASSERT_EQ(tablature.size(), 1U);
    EXPECT_EQ(tablature[0]->getStringAttribute("print-object"), "no");
    EXPECT_EQ(text(tablature[0], "type"), "whole");
}

TEST(MusicXmlTest, FNA01_ChordsWithSharpsAndTheirFrets)
{
    auto xml = parse(
        guitarScore({note(0, 48, {40, 47, 54}, {{.string = 0, .fret = 0}, {.string = 1, .fret = 2}, {}})}));
    ASSERT_NE(xml, nullptr);

    auto& bar = measure(*xml, 0, 0);
    const auto notes = notesOn(bar, 1);
    ASSERT_EQ(notes.size(), 3U);
    EXPECT_EQ(notes[0]->getChildByName("chord"), nullptr);
    EXPECT_NE(notes[1]->getChildByName("chord"), nullptr);
    EXPECT_EQ(text(notes[0], "pitch/step"), "E");
    EXPECT_EQ(text(notes[0], "pitch/octave"), "2");
    EXPECT_EQ(text(notes[2], "pitch/step"), "F");
    EXPECT_EQ(text(notes[2], "pitch/alter"), "1");
    EXPECT_EQ(text(notes[2], "pitch/octave"), "3");
    EXPECT_EQ(notes[0]->getChildByName("notations"), nullptr);
    EXPECT_EQ(text(notes[0], "voice"), "1");
    // The F# has no place on the fretboard: the tablature leaves it out.
    const auto frets = notesOn(bar, 2);
    ASSERT_EQ(frets.size(), 2U);
    EXPECT_EQ(text(frets[0], "notations/technical/string"), "6");
    EXPECT_EQ(text(frets[0], "notations/technical/fret"), "0");
    EXPECT_EQ(text(frets[1], "notations/technical/string"), "5");
    EXPECT_EQ(text(frets[1], "notations/technical/fret"), "2");
    EXPECT_EQ(text(frets[0], "voice"), "5");
}

TEST(MusicXmlTest, FNA01_ANoteWithoutAPlaceOnTheFretboardIsAHiddenRestInTheTablature)
{
    // The second pitch has no placement at all.
    auto xml = parse(guitarScore({note(0, 48, {20, 41}, {{}})}));
    ASSERT_NE(xml, nullptr);

    const auto frets = notesOn(measure(*xml, 0, 0), 2);
    ASSERT_EQ(frets.size(), 1U);
    EXPECT_NE(frets[0]->getChildByName("rest"), nullptr);
    EXPECT_EQ(frets[0]->getStringAttribute("print-object"), "no");
}

TEST(MusicXmlTest, FNA01_GraceNotesComeBeforeTheirNoteOnBothStaves)
{
    auto event = openLowE(12, 36);
    event.graces = {{.pitches = {42, 47}, .shape = {{.string = 0, .fret = 2}, {.string = 1, .fret = 2}}}};
    auto xml = parse(guitarScore({event}));
    ASSERT_NE(xml, nullptr);

    auto& bar = measure(*xml, 0, 0);
    const auto notes = notesOn(bar, 1);
    ASSERT_EQ(notes.size(), 4U);
    EXPECT_NE(notes[0]->getChildByName("rest"), nullptr);
    EXPECT_EQ(notes[1]->getChildByName("grace")->getStringAttribute("slash"), "yes");
    EXPECT_EQ(notes[1]->getChildByName("chord"), nullptr);
    EXPECT_EQ(notes[1]->getChildByName("duration"), nullptr);
    EXPECT_NE(notes[2]->getChildByName("chord"), nullptr);
    EXPECT_EQ(notes[2]->getChildByName("notations"), nullptr);
    EXPECT_EQ(notes[3]->getChildByName("grace"), nullptr);
    const auto frets = notesOn(bar, 2);
    ASSERT_EQ(frets.size(), 4U);
    EXPECT_EQ(text(frets[1], "notations/technical/fret"), "2");
}

TEST(MusicXmlTest, FNA01_EighthsAreBeamedPerBeat)
{
    auto xml = parse(guitarScore({openLowE(0, 6),
                                  note(6, 6, {40, 45}, {{.string = 0, .fret = 0}, {.string = 1, .fret = 0}}),
                                  openLowE(12, 3),
                                  openLowE(15, 3),
                                  openLowE(18, 6),
                                  openLowE(24, 6),
                                  openLowE(36, 12)}));
    ASSERT_NE(xml, nullptr);

    const auto notes = notesOn(measure(*xml, 0, 0), 1);
    ASSERT_EQ(notes.size(), 9U);
    EXPECT_EQ(text(notes[0], "beam"), "begin");
    EXPECT_EQ(text(notes[1], "beam"), "end");
    EXPECT_EQ(notes[2]->getChildByName("beam"), nullptr);
    EXPECT_EQ(text(notes[3], "beam"), "begin");
    EXPECT_EQ(text(notes[4], "beam"), "continue");
    EXPECT_EQ(text(notes[5], "beam"), "end");
    // An eighth before a rest and the quarter have no beam, nor have the notes of the tablature.
    EXPECT_EQ(notes[6]->getChildByName("beam"), nullptr);
    EXPECT_NE(notes[7]->getChildByName("rest"), nullptr);
    EXPECT_EQ(notes[8]->getChildByName("beam"), nullptr);
    for (const auto* fret : notesOn(measure(*xml, 0, 0), 2))
        EXPECT_EQ(fret->getChildByName("beam"), nullptr);
}

TEST(MusicXmlTest, FNA01_TripletsAreBracketedPerBeat)
{
    auto xml = parse(
        guitarScore({openLowE(0, 4), openLowE(4, 4), openLowE(8, 4), openLowE(12, 8), openLowE(24, 24)}));
    ASSERT_NE(xml, nullptr);

    const auto notes = notesOn(measure(*xml, 0, 0), 1);
    ASSERT_EQ(notes.size(), 6U);
    EXPECT_EQ(text(notes[0], "time-modification/actual-notes"), "3");
    EXPECT_EQ(text(notes[0], "time-modification/normal-notes"), "2");
    EXPECT_EQ(notes[0]->getChildByName("notations")->getChildByName("tuplet")->getStringAttribute("type"),
              "start");
    EXPECT_EQ(notes[1]->getChildByName("notations"), nullptr);
    EXPECT_EQ(notes[2]->getChildByName("notations")->getChildByName("tuplet")->getStringAttribute("type"),
              "stop");
    // A triplet quarter and a triplet eighth rest in the next beat: a bracket of their own.
    EXPECT_EQ(text(notes[3], "type"), "quarter");
    const auto* bracket = notes[3]->getChildByName("notations")->getChildByName("tuplet");
    EXPECT_EQ(bracket->getStringAttribute("type"), "start");
    EXPECT_EQ(bracket->getNextElementWithTagName("tuplet"), nullptr);
    EXPECT_NE(notes[4]->getChildByName("rest"), nullptr);
    EXPECT_EQ(notes[4]->getChildByName("notations")->getChildByName("tuplet")->getStringAttribute("type"),
              "stop");
    EXPECT_EQ(notes[5]->getChildByName("time-modification"), nullptr);
    // The tablature has the triplet values without brackets.
    const auto frets = notesOn(measure(*xml, 0, 0), 2);
    ASSERT_EQ(frets.size(), 6U);
    EXPECT_NE(frets[0]->getChildByName("time-modification"), nullptr);
    EXPECT_EQ(frets[0]->getChildByName("notations")->getChildByName("tuplet"), nullptr);
    EXPECT_EQ(frets[4]->getChildByName("notations"), nullptr);
}

TEST(MusicXmlTest, FNA01_TripletRestsShareTheBracket)
{
    auto xml = parse(guitarScore({openLowE(0, 12), openLowE(16, 4), openLowE(24, 24)}));
    ASSERT_NE(xml, nullptr);

    const auto notes = notesOn(measure(*xml, 0, 0), 1);
    ASSERT_EQ(notes.size(), 5U);
    // A triplet eighth rest, the note and a second rest: the rests open and close the bracket.
    EXPECT_EQ(notes[1]->getChildByName("notations")->getChildByName("tuplet")->getStringAttribute("type"),
              "start");
    EXPECT_EQ(notes[2]->getChildByName("notations"), nullptr);
    EXPECT_EQ(notes[3]->getChildByName("notations")->getChildByName("tuplet")->getStringAttribute("type"),
              "stop");
}

TEST(MusicXmlTest, FNA02_EveryVoiceIsAPartWithTheSameBars)
{
    auto score = guitarScore({openLowE(0, 48)});
    score.voices.push_back(
        {.name = "Bass & Co", .clef = model::Clef::bass8vb, .fretboard = kGuitar, .events = {}});
    auto xml = parse(score);
    ASSERT_NE(xml, nullptr);

    const auto* parts = xml->getChildByName("part-list");
    ASSERT_EQ(parts->getNumChildElements(), 2);
    EXPECT_EQ(parts->getChildElement(1)->getStringAttribute("id"), "P2");
    EXPECT_EQ(text(parts->getChildElement(1), "part-name"), "Bass & Co");
    EXPECT_EQ(measure(*xml, 1, 0).getStringAttribute("number"), "1");
}

TEST(MusicXmlTest, FNA01_VerovioDrawsTheScore)
{
    const auto score = guitarScore(
        {openLowE(0, 4), openLowE(4, 4), openLowE(8, 4), openLowE(12, 6), openLowE(18, 42)}, 0, 2);
    ScoreRenderer renderer{SNOTES_VEROVIO_DATA_DIR};

    ASSERT_EQ(renderer.load(toMusicXml(score)), 1);
    const auto svg = renderer.page(1);

    EXPECT_THAT(svg, HasSubstr("class=\"tabGrp"));
    EXPECT_THAT(svg, HasSubstr("class=\"beam"));
    EXPECT_THAT(svg, HasSubstr("class=\"tuplet"));
    EXPECT_THAT(svg, HasSubstr("class=\"tie"));
}

} // namespace
} // namespace snotes::notation
