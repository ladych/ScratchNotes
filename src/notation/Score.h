#pragma once

#include "model/InstrumentProfile.h"
#include "rhythm/Quantizer.h"
#include "tab/Tablature.h"

#include <cstdint>
#include <string>
#include <vector>

namespace snotes::notation
{

/// Ticks per beat: the quantizer places notes on quarters, eighths, sixteenths and triplets, so every
/// position and duration is a whole number of twelfths of a beat.
inline constexpr std::int64_t kTicksPerBeat = 12;

/// Notes struck together and where they are played, in the order of the pitches.
struct ScoreChord
{
    std::vector<int> pitches;
    tab::Shape shape;

    bool operator==(const ScoreChord&) const = default;
};

/// A note or chord of a voice in ticks from the start of the score; it ends at the latest where the next
/// one starts.
struct ScoreEvent
{
    std::int64_t startTick = 0;
    std::int64_t durationTicks = 0;
    ScoreChord chord;
    /// Grace notes just before the chord.
    std::vector<ScoreChord> graces;

    bool operator==(const ScoreEvent&) const = default;
};

/// One staff of notes and one of tablature (F-NA-01).
struct ScoreVoice
{
    std::string name;
    model::Clef clef = model::Clef::treble8vb;
    tab::Fretboard fretboard;
    std::vector<ScoreEvent> events;

    bool operator==(const ScoreVoice&) const = default;
};

/// All voices in common bars of beatsPerBar quarter notes (F-NA-02).
struct Score
{
    int beatsPerBar = 4;
    /// Length of the upbeat before bar 1; 0 if the score starts with bar 1.
    std::int64_t pickupTicks = 0;
    /// Full bars after the upbeat.
    int bars = 0;
    std::vector<ScoreVoice> voices;
    /// Where the score starts on the beat grid, in ticks from its first beat; a whole number of beats.
    std::int64_t originTick = 0;
    /// Tempo for players such as MuseScore (F-EX-02), beats per minute; 0 for none.
    double bpm = 0.0;

    [[nodiscard]] std::int64_t barTicks() const { return beatsPerBar * kTicksPerBeat; }

    bool operator==(const Score&) const = default;
};

/// What a voice brings into the score: its events on the beat grid with their tablature, one EventTab per
/// event, in the order of their positions.
struct VoiceEvents
{
    std::string name;
    model::Clef clef = model::Clef::treble8vb;
    tab::Fretboard fretboard;
    std::vector<rhythm::QuantizedEvent> events;
    std::vector<tab::EventTab> tabs;
};

/// A meter on the beat grid: bars of beatsPerBar beats, one of them starting at beat index phase.
struct Meter
{
    int beatsPerBar = 4;
    int phase = 0;
};

/// Lays the voices out in the bars of the meter. The silence before the first note of any voice is cut:
/// bar 1 starts at the first downbeat, and notes before it make an upbeat from the beat of the first note
/// on (Alex, 21.10.2026). Without any event the score is one empty bar.
[[nodiscard]] Score buildScore(const std::vector<VoiceEvents>& voices, const Meter& meter);

} // namespace snotes::notation
