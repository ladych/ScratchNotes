#pragma once

#include "rhythm/CleanUp.h"

#include <array>
#include <compare>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace snotes::rhythm
{

/// An exact position or duration in beats, always in lowest terms with a positive denominator.
class Fraction
{
public:
    Fraction() = default;
    Fraction(std::int64_t numerator, std::int64_t denominator = 1);

    [[nodiscard]] std::int64_t numerator() const { return numerator_; }
    [[nodiscard]] std::int64_t denominator() const { return denominator_; }
    /// "3/2", or "2" for whole numbers, as Python's fractions.Fraction.
    [[nodiscard]] std::string toString() const;

    friend Fraction operator+(const Fraction& a, const Fraction& b);
    friend Fraction operator-(const Fraction& a, const Fraction& b);
    friend bool operator==(const Fraction&, const Fraction&) = default;
    friend std::strong_ordering operator<=>(const Fraction& a, const Fraction& b);

private:
    std::int64_t numerator_ = 0;
    std::int64_t denominator_ = 1;
};

/// The continuous beat position of a time: 0 at the first beat, 1 at the second, linear in between and
/// extrapolated with the first and last beat interval beyond the ends (PoC beat_pos). beats needs two times.
[[nodiscard]] double beatPosition(double seconds, std::span<const double> beats);

/// A cleaned cluster on the beat grid (F-TX-09).
struct QuantizedEvent
{
    Fraction position;
    Fraction duration;
    double seconds = 0.0;
    double weight = 0.0;
    std::vector<int> pitches;
    std::vector<std::vector<int>> graces;

    bool operator==(const QuantizedEvent&) const = default;
};

/// The subdivisions a beat may use, cheapest first.
inline constexpr std::array<int, 4> kGrids{1, 2, 4, 3};

struct Quantization
{
    std::vector<QuantizedEvent> events;
    /// How many beats used each grid of kGrids.
    std::array<int, 4> gridUse{};

    bool operator==(const Quantization&) const = default;
};

/// Quantizes per beat to the grid of quarter, eighth, sixteenth or triplet notes with the lowest cost:
/// squared distance to the grid in units of 70 ms, plus a complexity penalty per grid, extra for a lone
/// triplet and for events that would land on the same position (PoC quantize). Events on the same position
/// keep the heaviest. A duration reaches the end of the notes in the grid of the onset; gaps up to half a
/// beat are closed (legato).
[[nodiscard]] Quantization quantize(const std::vector<Cluster>& clusters, std::span<const double> beats);

} // namespace snotes::rhythm
