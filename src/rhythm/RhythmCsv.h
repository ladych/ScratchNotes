#pragma once

#include "rhythm/RhythmAnalysis.h"

#include <optional>
#include <string>

namespace snotes::rhythm
{

/// beat,seconds,bpm: the common beat grid with the tempo from each beat to the next (F-TX-05).
[[nodiscard]] std::string beatsToCsv(const BeatGrid& grid);
/// beats_per_bar,confidence,phase: the meter suggestions, most likely first (F-TX-06).
[[nodiscard]] std::string meterToCsv(const BeatGrid& grid);
/// position,duration,seconds,weight,pitches,graces: the quantized events of a voice (F-TX-09). Positions
/// and durations in beats as fractions, pitches separated by spaces, grace notes by |.
[[nodiscard]] std::string eventsToCsv(const VoiceRhythm& rhythm);
/// The onsets of a take: a header line "hop,values", then one line per hop (kBeatHop, kDetailHop) with
/// the hop and the values.
[[nodiscard]] std::string onsetsToCsv(const Onsets& onsets);
/// Nothing if the text is not what onsetsToCsv wrote.
[[nodiscard]] std::optional<Onsets> onsetsFromCsv(const std::string& csv);

} // namespace snotes::rhythm
