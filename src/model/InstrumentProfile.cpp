#include "model/InstrumentProfile.h"

#include "i18n/Translate.h"

#include <algorithm>

namespace snotes::model
{
namespace
{

constexpr int kGuitarFrets = 24;
constexpr int kBassFrets = 24;

// MIDI notes: E2 = 40, G#1 = 32, E1 = 28, G#0 = 20.
const std::vector<InstrumentProfile> kProfiles{
    {.id = "guitar6-e",
     .name = i18n::marked("Guitar 6 strings, E standard"),
     .strings = {40, 45, 50, 55, 59, 64},
     .frets = kGuitarFrets,
     .clef = Clef::treble8vb},
    {.id = "guitar7-gs",
     .name = i18n::marked("Guitar 7 strings, G# standard"),
     .strings = {32, 37, 42, 47, 52, 56, 61},
     .frets = kGuitarFrets,
     .clef = Clef::treble8vb},
    {.id = "bass4-e",
     .name = i18n::marked("Bass 4 strings, E standard"),
     .strings = {28, 33, 38, 43},
     .frets = kBassFrets,
     .clef = Clef::bass8vb,
     .transcribeOctaveUp = true},
    {.id = "bass5-gs",
     .name = i18n::marked("Bass 5 strings, G# standard"),
     .strings = {20, 25, 30, 35, 40},
     .frets = kBassFrets,
     .clef = Clef::bass8vb,
     .transcribeOctaveUp = true},
};

} // namespace

const std::vector<InstrumentProfile>& instrumentProfiles()
{
    return kProfiles;
}

std::optional<InstrumentProfile> findInstrumentProfile(std::string_view id)
{
    const auto found = std::ranges::find(kProfiles, id, &InstrumentProfile::id);
    if (found == kProfiles.end())
        return std::nullopt;
    return *found;
}

const InstrumentProfile& defaultInstrumentProfile()
{
    return kProfiles.front();
}

} // namespace snotes::model
