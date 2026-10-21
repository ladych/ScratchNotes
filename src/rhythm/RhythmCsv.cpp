#include "rhythm/RhythmCsv.h"

#include "rhythm/OnsetStrength.h"

#include <cstddef>
#include <iomanip>
#include <locale>
#include <sstream>
#include <utility>

namespace snotes::rhythm
{
namespace
{

std::ostringstream csvStream()
{
    std::ostringstream csv;
    // Decimal points regardless of the user's locale.
    csv.imbue(std::locale::classic());
    csv << std::fixed;
    return csv;
}

void writePitches(std::ostream& csv, const std::vector<int>& pitches)
{
    for (std::size_t index = 0; index < pitches.size(); ++index)
        csv << (index == 0 ? "" : " ") << pitches[index];
}

} // namespace

std::string beatsToCsv(const BeatGrid& grid)
{
    auto csv = csvStream();
    csv << "beat,seconds,bpm\n";
    const auto tempo = tempoCurve(grid.beats);
    for (std::size_t beat = 0; beat < grid.beats.size(); ++beat)
    {
        csv << beat << ',' << std::setprecision(4) << grid.beats[beat] << ',';
        if (beat < tempo.size())
            csv << std::setprecision(2) << tempo[beat];
        csv << '\n';
    }
    return csv.str();
}

std::string meterToCsv(const BeatGrid& grid)
{
    auto csv = csvStream();
    csv << "beats_per_bar,confidence,phase\n" << std::setprecision(3);
    for (const auto& candidate : grid.meter.ranking)
        csv << candidate.beatsPerBar << ',' << candidate.confidence << ',' << candidate.phase << '\n';
    return csv.str();
}

std::string eventsToCsv(const VoiceRhythm& rhythm)
{
    auto csv = csvStream();
    csv << "position,duration,seconds,weight,pitches,graces\n" << std::setprecision(4);
    for (const auto& event : rhythm.quantization.events)
    {
        csv << event.position.toString() << ',' << event.duration.toString() << ',' << event.seconds << ','
            << event.weight << ',';
        writePitches(csv, event.pitches);
        csv << ',';
        for (std::size_t grace = 0; grace < event.graces.size(); ++grace)
        {
            csv << (grace == 0 ? "" : "|");
            writePitches(csv, event.graces[grace]);
        }
        csv << '\n';
    }
    return csv.str();
}

namespace
{

void writeEnvelope(std::ostream& csv, int hop, const std::vector<float>& values)
{
    csv << hop;
    for (const float value : values)
        csv << ',' << value;
    csv << '\n';
}

/// A line "hop,value,value,...".
std::optional<std::vector<float>> readEnvelope(std::istream& lines, int hop)
{
    std::string line;
    std::getline(lines, line);
    std::istringstream fields{line};
    fields.imbue(std::locale::classic());
    int readHop = 0;
    fields >> readHop;
    if (fields.fail() || readHop != hop)
        return std::nullopt;
    std::vector<float> values;
    char comma = 0;
    for (float value = 0.0F; fields >> comma >> value;)
        values.push_back(value);
    return values;
}

} // namespace

std::string onsetsToCsv(const Onsets& onsets)
{
    std::ostringstream csv;
    csv.imbue(std::locale::classic());
    csv << "hop,values\n" << std::setprecision(6);
    writeEnvelope(csv, kBeatHop, onsets.beat);
    writeEnvelope(csv, kDetailHop, onsets.detail);
    return csv.str();
}

std::optional<Onsets> onsetsFromCsv(const std::string& csv)
{
    std::istringstream lines{csv};
    std::string header;
    std::getline(lines, header);
    auto beat = readEnvelope(lines, kBeatHop);
    auto detail = readEnvelope(lines, kDetailHop);
    if (!beat || !detail)
        return std::nullopt;
    return Onsets{.beat = std::move(*beat), .detail = std::move(*detail)};
}

} // namespace snotes::rhythm
