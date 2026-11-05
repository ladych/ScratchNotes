#pragma once

#include "model/InstrumentProfile.h"
#include "rhythm/Quantizer.h"

#include <span>
#include <vector>

namespace snotes::tab
{

/// The strings of an instrument as MIDI notes from the lowest to the highest one, and its frets.
struct Fretboard
{
    std::vector<int> strings;
    int frets = 0;

    bool operator==(const Fretboard&) const = default;
};

[[nodiscard]] Fretboard fretboardOf(const model::InstrumentProfile& profile);

/// Where a note is played. string counts from 0 at the lowest string; kUnplayable if the note has no place
/// on the fretboard or no free string in its chord.
struct Placement
{
    static constexpr int kUnplayable = -1;

    int string = kUnplayable;
    int fret = 0;

    bool operator==(const Placement&) const = default;
};

/// The placements of the notes of a chord, in the order of its pitches.
using Shape = std::vector<Placement>;

/// Every way to play the pitches with one note per string and at most kMaxStretch frets between the lowest
/// and the highest fretted note (open strings stretch nothing). A pitch without any place stays unplayable in
/// all shapes; if no shape fits, the shapes leave the fewest notes unplayable.
[[nodiscard]] std::vector<Shape> playableShapes(std::span<const int> pitches, const Fretboard& fretboard);

inline constexpr int kMaxStretch = 4;

/// Fixed strings of the pitches of a chord, in their order, kUnplayable for a pitch whose string is free;
/// empty if all are free.
using Strings = std::vector<int>;

/// Notes struck together at a time.
struct Chord
{
    double seconds = 0.0;
    std::vector<int> pitches;
    /// The strings the user chose (F-TB-03).
    Strings strings;
};

/// The cheapest way through the chords (F-TB-02): dynamic programming over the shapes of each chord, where
/// moving the hand costs per fret (less after a pause), wide and high shapes cost a little and unplayable
/// notes a lot. Open strings keep the hand where it is. A chord with fixed strings only takes shapes that
/// play its pitches there, if there is one; the rest of the way adapts to it (F-TB-03).
[[nodiscard]] std::vector<Shape> fingering(const std::vector<Chord>& chords, const Fretboard& fretboard);

/// The tablature of a quantized event and its grace notes.
struct EventTab
{
    Shape notes;
    std::vector<Shape> graces;

    bool operator==(const EventTab&) const = default;
};

/// String and fret for every note of the events, grace notes included, in one fingering (F-TB-01). strings
/// has the fixed strings of the events in their order (F-TB-03); events beyond it are free.
[[nodiscard]] std::vector<EventTab> tabulate(const std::vector<rhythm::QuantizedEvent>& events,
                                             const Fretboard& fretboard,
                                             const std::vector<Strings>& strings = {});

} // namespace snotes::tab
