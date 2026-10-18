#include "rhythm/Quantizer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <numeric>
#include <set>

namespace snotes::rhythm
{

Fraction::Fraction(std::int64_t numerator, std::int64_t denominator)
{
    const auto divisor = std::gcd(numerator, denominator);
    const std::int64_t sign = denominator < 0 ? -1 : 1;
    numerator_ = sign * numerator / divisor;
    denominator_ = sign * denominator / divisor;
}

std::string Fraction::toString() const
{
    if (denominator_ == 1)
        return std::to_string(numerator_);
    return std::to_string(numerator_) + "/" + std::to_string(denominator_);
}

Fraction operator+(const Fraction& a, const Fraction& b)
{
    return {(a.numerator_ * b.denominator_) + (b.numerator_ * a.denominator_),
            a.denominator_ * b.denominator_};
}

Fraction operator-(const Fraction& a, const Fraction& b)
{
    return a + Fraction{-b.numerator_, b.denominator_};
}

std::strong_ordering operator<=>(const Fraction& a, const Fraction& b)
{
    return a.numerator_ * b.denominator_ <=> b.numerator_ * a.denominator_;
}

double beatPosition(double seconds, std::span<const double> beats)
{
    const std::size_t last = beats.size() - 1;
    if (seconds < beats[0])
        return (seconds - beats[0]) / (beats[1] - beats[0]);
    if (seconds > beats[last])
        return static_cast<double>(last) + ((seconds - beats[last]) / (beats[last] - beats[last - 1]));
    const auto after = static_cast<std::size_t>(std::ranges::upper_bound(beats, seconds) - beats.begin());
    if (after > last)
        return static_cast<double>(last);
    return static_cast<double>(after - 1) +
           ((seconds - beats[after - 1]) / (beats[after] - beats[after - 1]));
}

namespace
{

constexpr std::array<double, 4> kGridPenalty{0.0, 0.5, 1.5, 2.0};
constexpr double kSigma = 0.07;
constexpr double kLoneTripletPenalty = 2.0;
constexpr double kCollisionPenalty = 3.0;
/// Onsets a little before a beat belong to it.
constexpr double kBeatLead = 0.125;

struct Placed
{
    const Cluster* cluster = nullptr;
    double position = 0.0;
    Fraction quantized;
};

/// The cheapest grid for the onsets of one beat; returns its index in kGrids.
std::size_t quantizeBeat(std::vector<Placed*>& onsets, std::int64_t beat)
{
    double bestCost = std::numeric_limits<double>::infinity();
    std::size_t best = 0;
    for (std::size_t grid = 0; grid < kGrids.size(); ++grid)
    {
        const auto steps = static_cast<double>(kGrids.at(grid));
        double cost =
            kGridPenalty.at(grid) + (kGrids.at(grid) == 3 && onsets.size() == 1 ? kLoneTripletPenalty : 0.0);
        std::set<double> positions;
        for (const auto* onset : onsets)
        {
            const double fraction = onset->position - static_cast<double>(beat);
            const double snapped = std::nearbyint(fraction * steps) / steps;
            cost += (fraction - snapped) * (fraction - snapped) / (kSigma * kSigma);
            positions.insert(snapped);
        }
        cost += kCollisionPenalty * static_cast<double>(onsets.size() - positions.size());
        if (cost < bestCost)
        {
            bestCost = cost;
            best = grid;
        }
    }
    const int steps = kGrids.at(best);
    for (auto* onset : onsets)
        onset->quantized =
            Fraction{beat} + Fraction{static_cast<std::int64_t>(std::nearbyint(
                                          (onset->position - static_cast<double>(beat)) * steps)),
                                      steps};
    return best;
}

Fraction snappedEnd(double end, const Fraction& position)
{
    const std::int64_t steps = position.denominator() == 3 ? 3 : 2;
    return std::max(
        Fraction{static_cast<std::int64_t>(std::nearbyint(end * static_cast<double>(steps))), steps},
        position + Fraction{1, steps});
}

/// The heaviest onset on each position, in the order of the positions.
std::map<Fraction, const Placed*> heaviestPerPosition(const std::vector<Placed>& placed)
{
    std::map<Fraction, const Placed*> heaviest;
    for (const auto& onset : placed)
    {
        const auto [entry, added] = heaviest.try_emplace(onset.quantized, &onset);
        if (!added && onset.cluster->weight > entry->second->cluster->weight)
            entry->second = &onset;
    }
    return heaviest;
}

} // namespace

Quantization quantize(const std::vector<Cluster>& clusters, std::span<const double> beats)
{
    std::vector<Placed> placed;
    placed.reserve(clusters.size());
    for (const auto& cluster : clusters)
        placed.push_back(
            {.cluster = &cluster, .position = beatPosition(cluster.seconds, beats), .quantized = {}});
    std::map<std::int64_t, std::vector<Placed*>> byBeat;
    for (auto& onset : placed)
        byBeat[static_cast<std::int64_t>(std::floor(onset.position + kBeatLead))].push_back(&onset);
    Quantization result;
    for (auto& [beat, onsets] : byBeat)
        ++result.gridUse.at(quantizeBeat(onsets, beat));
    const auto heaviest = heaviestPerPosition(placed);
    for (auto entry = heaviest.begin(); entry != heaviest.end(); ++entry)
    {
        const auto& [position, onset] = *entry;
        const auto end = snappedEnd(beatPosition(onset->cluster->endSeconds, beats), position);
        Fraction duration = std::max(end - position, Fraction{1, 2});
        if (const auto next = std::next(entry); next != heaviest.end())
            duration = next->first - end <= Fraction{1, 2} ? next->first - position : end - position;
        result.events.push_back({.position = position,
                                 .duration = duration,
                                 .seconds = onset->cluster->seconds,
                                 .weight = onset->cluster->weight,
                                 .pitches = onset->cluster->pitches,
                                 .graces = onset->cluster->graces});
    }
    return result;
}

} // namespace snotes::rhythm
