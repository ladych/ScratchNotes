#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace snotes::model
{

enum class Clef : std::uint8_t
{
    /// Treble clef an octave down, as written for guitar.
    treble8vb,
    /// Bass clef an octave down, as written for bass.
    bass8vb
};

/// An instrument with its tuning (F-AU-01, F-TB-01). The profile decides the note range of the
/// transcription and the strings of the tablature.
struct InstrumentProfile
{
    std::string id;
    /// English name, marked for translation; show it through i18n::tr.
    std::string name;
    /// Open strings as MIDI notes from the lowest to the highest string.
    std::vector<int> strings;
    int frets = 0;
    Clef clef = Clef::treble8vb;
};

/// The profiles of the MVP (Lastenheft 7, decided 15.10.2026): guitar 6 in E standard, guitar 7 in
/// G#1 standard, bass 4 in E standard and bass 5 in G#0 standard.
[[nodiscard]] const std::vector<InstrumentProfile>& instrumentProfiles();
[[nodiscard]] std::optional<InstrumentProfile> findInstrumentProfile(std::string_view id);
/// The profile a new voice starts with.
[[nodiscard]] const InstrumentProfile& defaultInstrumentProfile();

} // namespace snotes::model
