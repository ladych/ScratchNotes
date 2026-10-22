#pragma once

#include "rhythm/RhythmAnalysis.h"
#include "tab/Tablature.h"

#include <cstddef>
#include <optional>
#include <string>

namespace snotes::rhythm
{

/// beat,seconds,bpm: the common beat grid with the tempo from each beat to the next (F-TX-05).
[[nodiscard]] std::string beatsToCsv(const BeatGrid& grid);
/// beats_per_bar,confidence,phase: the meter suggestions, most likely first (F-TX-06).
[[nodiscard]] std::string meterToCsv(const BeatGrid& grid);
/// position,duration,seconds,weight,pitches,graces,tab,grace_tab: the quantized events of a voice (F-TX-09)
/// with their tablature on an instrument with that many strings (F-TB-01). Positions and durations in beats
/// as fractions, pitches separated by spaces, grace notes by |, tab as tab::shapeToText. tabs has one entry
/// per event.
[[nodiscard]] std::string
eventsToCsv(const VoiceRhythm& rhythm, const std::vector<tab::EventTab>& tabs, std::size_t strings);
/// The onsets of a take: a header line "hop,values", then one line per hop (kBeatHop, kDetailHop) with
/// the hop and the values.
[[nodiscard]] std::string onsetsToCsv(const Onsets& onsets);
/// Nothing if the text is not what onsetsToCsv wrote.
[[nodiscard]] std::optional<Onsets> onsetsFromCsv(const std::string& csv);

} // namespace snotes::rhythm
