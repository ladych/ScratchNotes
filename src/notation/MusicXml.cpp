#include "notation/MusicXml.h"

#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace snotes::notation
{
namespace
{

constexpr std::array<NoteValue, 7> kInBeat{{{.ticks = 9, .type = "eighth", .dots = 1, .triplet = false},
                                            {.ticks = 8, .type = "quarter", .dots = 0, .triplet = true},
                                            {.ticks = 6, .type = "eighth", .dots = 0, .triplet = false},
                                            {.ticks = 4, .type = "eighth", .dots = 0, .triplet = true},
                                            {.ticks = 3, .type = "16th", .dots = 0, .triplet = false},
                                            {.ticks = 2, .type = "16th", .dots = 0, .triplet = true},
                                            {.ticks = 1, .type = "32nd", .dots = 0, .triplet = true}}};
constexpr std::array<NoteValue, 4> kWholeBeats{
    {{.ticks = 48, .type = "whole", .dots = 0, .triplet = false},
     {.ticks = 36, .type = "half", .dots = 1, .triplet = false},
     {.ticks = 24, .type = "half", .dots = 0, .triplet = false},
     {.ticks = 12, .type = "quarter", .dots = 0, .triplet = false}}};

constexpr std::array<const char*, 12> kSteps{"C", "C", "D", "D", "E", "F", "F", "G", "G", "A", "A", "B"};
constexpr std::array<bool, 12> kSharp{
    false, true, false, true, false, false, true, false, true, false, true, false};

/// The values from the table, longest first, that add up to length; the table ends with one that fits.
void appendValues(std::vector<NoteValue>& values, std::int64_t length, std::span<const NoteValue> table)
{
    while (length > 0)
    {
        const auto& value =
            *std::ranges::find_if(table, [length](const NoteValue& v) { return v.ticks <= length; });
        values.push_back(value);
        length -= value.ticks;
    }
}

struct Measure
{
    std::int64_t start = 0;
    std::int64_t length = 0;
    /// 0 for the upbeat.
    int number = 0;
};

std::vector<Measure> measuresOf(const Score& score)
{
    std::vector<Measure> measures;
    measures.reserve(static_cast<std::size_t>(score.bars) + 1);
    if (score.pickupTicks > 0)
        measures.push_back({.start = 0, .length = score.pickupTicks, .number = 0});
    for (int bar = 0; bar < score.bars; ++bar)
        measures.push_back({.start = score.pickupTicks + bar * score.barTicks(),
                            .length = score.barTicks(),
                            .number = bar + 1});
    return measures;
}

/// A stretch of a bar with one event or a rest (event nullptr); start counts from the start of the bar.
struct Segment
{
    std::int64_t start = 0;
    std::int64_t length = 0;
    const ScoreEvent* event = nullptr;
    bool tiedFromBefore = false;
    bool tiedOn = false;
};

std::vector<Segment> segmentsOf(const std::vector<ScoreEvent>& events, const Measure& measure)
{
    std::vector<Segment> segments;
    const auto end = measure.start + measure.length;
    auto position = measure.start;
    for (const auto& event : events)
    {
        const auto eventEnd = event.startTick + event.durationTicks;
        if (eventEnd <= measure.start || event.startTick >= end)
            continue;
        const auto from = std::max(event.startTick, measure.start);
        const auto to = std::min(eventEnd, end);
        const bool tiedFromBefore = event.startTick < measure.start;
        const bool tiedOn = eventEnd > end;
        if (from > position)
            segments.push_back({.start = position - measure.start, .length = from - position});
        segments.push_back(Segment{from - measure.start, to - from, &event, tiedFromBefore, tiedOn});
        position = to;
    }
    if (position < end)
        segments.push_back({.start = position - measure.start, .length = end - position});
    return segments;
}

enum class Beam : std::uint8_t
{
    none,
    begin,
    proceed,
    end
};

/// One written note value of a segment.
struct Piece
{
    /// From the start of the bar.
    std::int64_t start = 0;
    /// From the start of the score.
    std::int64_t inScore = 0;
    NoteValue value;
    const ScoreEvent* event = nullptr;
    bool tieStop = false;
    bool tieStart = false;
    /// The first value of the event, which carries its grace notes.
    bool first = false;
    /// A rest over the whole bar.
    bool wholeBar = false;
    bool tupletStart = false;
    bool tupletStop = false;
    Beam beam = Beam::none;
};

std::int64_t beatOf(const Piece& piece)
{
    return piece.start / kTicksPerBeat;
}

bool beamable(const Piece& piece)
{
    return piece.event != nullptr && piece.value.ticks < kTicksPerBeat && piece.value.type != "quarter";
}

bool inOneBeam(const Piece& a, const Piece& b)
{
    return beamable(a) && beamable(b) && beatOf(a) == beatOf(b);
}

bool inOneTuplet(const Piece& a, const Piece& b)
{
    return a.value.triplet && b.value.triplet && beatOf(a) == beatOf(b);
}

Beam beamOf(bool before, bool after)
{
    if (before)
        return after ? Beam::proceed : Beam::end;
    return after ? Beam::begin : Beam::none;
}

/// Groups the beams and the triplet brackets per beat.
void group(std::vector<Piece>& pieces)
{
    for (std::size_t index = 0; index < pieces.size(); ++index)
    {
        auto& piece = pieces[index];
        const auto* previous = index > 0 ? &pieces[index - 1] : nullptr;
        const auto* next = index + 1 < pieces.size() ? &pieces[index + 1] : nullptr;
        piece.beam = beamOf(previous != nullptr && inOneBeam(*previous, piece),
                            next != nullptr && inOneBeam(piece, *next));
        piece.tupletStart = piece.value.triplet && (previous == nullptr || !inOneTuplet(*previous, piece));
        piece.tupletStop = piece.value.triplet && (next == nullptr || !inOneTuplet(piece, *next));
    }
}

std::vector<Piece> piecesOf(const std::vector<Segment>& segments, const Measure& measure)
{
    std::vector<Piece> pieces;
    if (measure.number > 0 && segments.size() == 1 && segments.front().event == nullptr)
    {
        pieces.push_back({.value = {.ticks = measure.length, .type = "whole"}, .wholeBar = true});
        return pieces;
    }
    for (const auto& segment : segments)
    {
        const auto values = noteValues(segment.start, segment.length);
        auto start = segment.start;
        for (std::size_t index = 0; index < values.size(); ++index)
        {
            pieces.push_back({.start = start,
                              .inScore = measure.start + start,
                              .value = values[index],
                              .event = segment.event,
                              .tieStop = index > 0 || segment.tiedFromBefore,
                              .tieStart = index + 1 < values.size() || segment.tiedOn,
                              .first = index == 0 && !segment.tiedFromBefore});
            start += values[index].ticks;
        }
    }
    group(pieces);
    return pieces;
}

/// The staff of notes (1) or of tablature (2) of a voice.
struct Staff
{
    int number = 1;
    std::size_t strings = 0;
    /// Index of the voice's part.
    std::size_t part = 0;

    [[nodiscard]] bool tablature() const { return number == 2; }
    [[nodiscard]] const char* voice() const { return tablature() ? "5" : "1"; }
};

struct ChordNote
{
    int pitch = 0;
    std::optional<tab::Placement> placement;
};

/// All notes on the staff of notes, the playable ones with their place on the tablature.
std::vector<ChordNote> notesOn(const ScoreChord& chord, const Staff& staff)
{
    std::vector<ChordNote> notes;
    notes.reserve(chord.pitches.size());
    for (std::size_t index = 0; index < chord.pitches.size(); ++index)
    {
        if (!staff.tablature())
            notes.push_back({.pitch = chord.pitches[index]});
        else if (index < chord.shape.size() && chord.shape[index].string != tab::Placement::kUnplayable)
            notes.push_back({.pitch = chord.pitches[index], .placement = chord.shape[index]});
    }
    return notes;
}

void addText(juce::XmlElement& parent, const char* tag, const juce::String& text)
{
    parent.createNewChildElement(tag)->addTextElement(text);
}

void addPitch(juce::XmlElement& note, int midi)
{
    auto* pitch = note.createNewChildElement("pitch");
    const auto pitchClass = static_cast<std::size_t>(midi % 12);
    addText(*pitch, "step", kSteps.at(pitchClass));
    if (kSharp.at(pitchClass))
        addText(*pitch, "alter", "1");
    addText(*pitch, "octave", juce::String{(midi / 12) - 1});
}

/// type, dots and the triplet ratio.
void addValue(juce::XmlElement& note, const NoteValue& value)
{
    addText(note, "type", juce::String{value.type.data(), value.type.size()});
    for (int dot = 0; dot < value.dots; ++dot)
        note.createNewChildElement("dot");
    if (!value.triplet)
        return;
    auto* ratio = note.createNewChildElement("time-modification");
    addText(*ratio, "actual-notes", "3");
    addText(*ratio, "normal-notes", "2");
}

void addTie(juce::XmlElement& parent, const char* tag, const char* type)
{
    parent.createNewChildElement(tag)->setAttribute("type", type);
}

void addTuplet(juce::XmlElement& notations, const char* type, bool tablature)
{
    auto* tuplet = notations.createNewChildElement("tuplet");
    tuplet->setAttribute("type", type);
    if (tablature)
    {
        tuplet->setAttribute("bracket", "no");
        tuplet->setAttribute("show-number", "none");
    }
}

/// On the tablature the triplets have no bracket and no number.
void addTupletBrackets(juce::XmlElement& notations, const Piece& piece, bool tablature = false)
{
    if (piece.tupletStart)
        addTuplet(notations, "start", tablature);
    if (piece.tupletStop)
        addTuplet(notations, "stop", tablature);
}

constexpr std::array<const char*, 4> kBeamNames{"", "begin", "continue", "end"};

void addBeam(juce::XmlElement& note, Beam beam)
{
    auto* element = note.createNewChildElement("beam");
    element->setAttribute("number", "1");
    element->addTextElement(kBeamNames.at(static_cast<std::size_t>(beam)));
}

void addTechnical(juce::XmlElement& notations, const tab::Placement& placement, const Staff& staff)
{
    auto* technical = notations.createNewChildElement("technical");
    addText(*technical, "string", juce::String{static_cast<int>(staff.strings) - placement.string});
    addText(*technical, "fret", juce::String{placement.fret});
}

/// staff, beam and notations of a note; leading tells whether it is the first note of its chord, which
/// carries the beam and the triplet bracket.
void addNoteTail(
    juce::XmlElement& note, const ChordNote& chordNote, const Piece& piece, const Staff& staff, bool leading)
{
    addText(note, "staff", juce::String{staff.number});
    if (leading && !staff.tablature() && piece.beam != Beam::none)
        addBeam(note, piece.beam);
    auto* notations = note.createNewChildElement("notations");
    if (piece.tieStop)
        addTie(*notations, "tied", "stop");
    if (piece.tieStart)
        addTie(*notations, "tied", "start");
    if (leading)
        addTupletBrackets(*notations, piece, staff.tablature());
    if (chordNote.placement)
        addTechnical(*notations, *chordNote.placement, staff);
    if (notations->getNumChildElements() == 0)
        note.removeChildElement(notations, true);
}

/// index is the place of the note in its chord.
void writeNote(juce::XmlElement& measure,
               const ChordNote& chordNote,
               const Piece& piece,
               const Staff& staff,
               std::size_t index)
{
    const bool leading = index == 0;
    auto* note = measure.createNewChildElement("note");
    note->setAttribute(
        "id",
        juce::String{noteId({.start = piece.inScore, .end = piece.inScore + piece.value.ticks})} + "_" +
            juce::String{static_cast<int>(staff.part)} + "_" + juce::String{staff.number} + "_" +
            juce::String{static_cast<int>(index)});
    if (!leading)
        note->createNewChildElement("chord");
    addPitch(*note, chordNote.pitch);
    addText(*note, "duration", juce::String{piece.value.ticks});
    if (piece.tieStop)
        addTie(*note, "tie", "stop");
    if (piece.tieStart)
        addTie(*note, "tie", "start");
    addText(*note, "voice", staff.voice());
    addValue(*note, piece.value);
    addNoteTail(*note, chordNote, piece, staff, leading);
}

void writeGrace(juce::XmlElement& measure, const ChordNote& chordNote, const Staff& staff, bool leading)
{
    auto* note = measure.createNewChildElement("note");
    note->createNewChildElement("grace")->setAttribute("slash", "yes");
    if (!leading)
        note->createNewChildElement("chord");
    addPitch(*note, chordNote.pitch);
    addText(*note, "voice", staff.voice());
    addText(*note, "type", "eighth");
    addText(*note, "staff", juce::String{staff.number});
    if (chordNote.placement)
        addTechnical(*note->createNewChildElement("notations"), *chordNote.placement, staff);
}

void writeRest(juce::XmlElement& measure, const Piece& piece, const Staff& staff)
{
    auto* note = measure.createNewChildElement("note");
    if (staff.tablature())
        note->setAttribute("print-object", "no");
    auto* rest = note->createNewChildElement("rest");
    if (piece.wholeBar)
        rest->setAttribute("measure", "yes");
    addText(*note, "duration", juce::String{piece.value.ticks});
    addText(*note, "voice", staff.voice());
    // Verovio leaks a hidden rest without a type.
    if (!piece.wholeBar || staff.tablature())
        addValue(*note, piece.value);
    addText(*note, "staff", juce::String{staff.number});
    if (piece.tupletStart || piece.tupletStop)
        addTupletBrackets(*note->createNewChildElement("notations"), piece, staff.tablature());
}

void writePiece(juce::XmlElement& measure, const Piece& piece, const Staff& staff)
{
    if (piece.event == nullptr)
    {
        writeRest(measure, piece, staff);
        return;
    }
    if (piece.first)
        for (const auto& grace : piece.event->graces)
        {
            const auto notes = notesOn(grace, staff);
            for (std::size_t index = 0; index < notes.size(); ++index)
                writeGrace(measure, notes[index], staff, index == 0);
        }
    const auto notes = notesOn(piece.event->chord, staff);
    if (notes.empty())
        writeRest(measure, piece, staff);
    for (std::size_t index = 0; index < notes.size(); ++index)
        writeNote(measure, notes[index], piece, staff, index);
}

void addClefs(juce::XmlElement& attributes, model::Clef clef)
{
    auto* notes = attributes.createNewChildElement("clef");
    notes->setAttribute("number", "1");
    addText(*notes, "sign", clef == model::Clef::bass8vb ? "F" : "G");
    addText(*notes, "line", clef == model::Clef::bass8vb ? "4" : "2");
    addText(*notes, "clef-octave-change", "-1");
    auto* tablature = attributes.createNewChildElement("clef");
    tablature->setAttribute("number", "2");
    addText(*tablature, "sign", "TAB");
    addText(*tablature, "line", "5");
}

/// The tuning of the tablature staff, line 1 at the bottom for the lowest string.
void addTuning(juce::XmlElement& attributes, const tab::Fretboard& fretboard)
{
    auto* details = attributes.createNewChildElement("staff-details");
    details->setAttribute("number", "2");
    addText(*details, "staff-lines", juce::String{static_cast<int>(fretboard.strings.size())});
    for (std::size_t string = 0; string < fretboard.strings.size(); ++string)
    {
        const auto open = fretboard.strings[string];
        auto* tuning = details->createNewChildElement("staff-tuning");
        tuning->setAttribute("line", static_cast<int>(string) + 1);
        addText(*tuning, "tuning-step", kSteps.at(static_cast<std::size_t>(open % 12)));
        if (kSharp.at(static_cast<std::size_t>(open % 12)))
            addText(*tuning, "tuning-alter", "1");
        addText(*tuning, "tuning-octave", juce::String{(open / 12) - 1});
    }
}

void addAttributes(juce::XmlElement& measure, const Score& score, const ScoreVoice& voice)
{
    auto* attributes = measure.createNewChildElement("attributes");
    addText(*attributes, "divisions", juce::String{kTicksPerBeat});
    addText(*attributes->createNewChildElement("key"), "fifths", "0");
    auto* time = attributes->createNewChildElement("time");
    addText(*time, "beats", juce::String{score.beatsPerBar});
    addText(*time, "beat-type", "4");
    addText(*attributes, "staves", "2");
    addClefs(*attributes, voice.clef);
    addTuning(*attributes, voice.fretboard);
}

/// A metronome mark for the reader and a tempo for players (F-EX-02).
void addTempo(juce::XmlElement& measure, double bpm)
{
    auto* direction = measure.createNewChildElement("direction");
    direction->setAttribute("placement", "above");
    auto* metronome = direction->createNewChildElement("direction-type")->createNewChildElement("metronome");
    addText(*metronome, "beat-unit", "quarter");
    addText(*metronome, "per-minute", juce::String{juce::roundToInt(bpm)});
    direction->createNewChildElement("sound")->setAttribute("tempo", juce::String{bpm, 2});
}

/// bpm is the tempo to mark at the start, 0 for none; index is the part's.
void writeMeasure(
    juce::XmlElement& part, const Score& score, std::size_t index, const Measure& measure, double bpm)
{
    const auto& voice = score.voices[index];
    auto* element = part.createNewChildElement("measure");
    element->setAttribute("number", measure.number);
    element->setAttribute("id", juce::String{kBarIdPrefix} + juce::String{measure.number});
    if (measure.number == 0)
        element->setAttribute("implicit", "yes");
    if (measure.start == 0)
        addAttributes(*element, score, voice);
    if (measure.start == 0 && bpm > 0.0)
        addTempo(*element, bpm);
    const auto pieces = piecesOf(segmentsOf(voice.events, measure), measure);
    for (const auto& piece : pieces)
        writePiece(*element, piece, {.number = 1, .strings = voice.fretboard.strings.size(), .part = index});
    addText(*element->createNewChildElement("backup"), "duration", juce::String{measure.length});
    for (const auto& piece : pieces)
        writePiece(*element, piece, {.number = 2, .strings = voice.fretboard.strings.size(), .part = index});
}

} // namespace

std::string noteId(const NoteSpan& span)
{
    return "n" + std::to_string(span.start) + "_" + std::to_string(span.end);
}

std::optional<NoteSpan> noteSpanOf(std::string_view id)
{
    NoteSpan span;
    if (!id.starts_with('n'))
        return std::nullopt;
    const auto* const end = id.data() + id.size();
    const auto start = std::from_chars(id.data() + 1, end, span.start);
    if (start.ec != std::errc{} || start.ptr == end || *start.ptr != '_')
        return std::nullopt;
    const auto stop = std::from_chars(start.ptr + 1, end, span.end);
    if (stop.ec != std::errc{})
        return std::nullopt;
    return span;
}

std::vector<NoteValue> noteValues(std::int64_t start, std::int64_t length)
{
    std::vector<NoteValue> values;
    const auto end = start + length;
    const auto nextBeat = (start + kTicksPerBeat - 1) / kTicksPerBeat * kTicksPerBeat;
    const auto head = std::min(end, nextBeat);
    appendValues(values, head - start, kInBeat);
    const auto lastBeat = std::max(head, end / kTicksPerBeat * kTicksPerBeat);
    appendValues(values, lastBeat - head, kWholeBeats);
    appendValues(values, end - lastBeat, kInBeat);
    return values;
}

} // namespace snotes::notation

namespace snotes::notation
{

std::string toMusicXml(const Score& score)
{
    juce::XmlElement root{"score-partwise"};
    root.setAttribute("version", "4.0");
    auto* parts = root.createNewChildElement("part-list");
    const auto measures = measuresOf(score);
    for (std::size_t index = 0; index < score.voices.size(); ++index)
    {
        const auto& voice = score.voices[index];
        const auto id = "P" + juce::String{static_cast<int>(index) + 1};
        auto* scorePart = parts->createNewChildElement("score-part");
        scorePart->setAttribute("id", id);
        addText(*scorePart, "part-name", juce::String::fromUTF8(voice.name.c_str()));
        auto* part = root.createNewChildElement("part");
        part->setAttribute("id", id);
        for (const auto& measure : measures)
            writeMeasure(*part, score, index, measure, index == 0 ? score.bpm : 0.0);
    }
    juce::XmlElement::TextFormat format;
    format.dtd = "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 4.0 Partwise//EN\" "
                 "\"http://www.musicxml.org/dtds/partwise.dtd\">";
    format.newLineChars = "\n";
    return root.toString(format).toStdString();
}

} // namespace snotes::notation
