#include "tab/Tablature.h"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <optional>
#include <ranges>
#include <utility>

namespace snotes::tab
{
namespace
{

/// A note no string can take costs more than any hand movement.
constexpr double kUnplayableCost = 100.0;
constexpr double kShiftCost = 1.0;
/// After this long without a note the hand moves at kPauseFactor of the cost.
constexpr double kPauseSeconds = 0.5;
constexpr double kPauseFactor = 0.5;
constexpr double kStretchCost = 0.3;
/// Per fret of the hand position: of two equal ways the lower one wins.
constexpr double kHeightCost = 0.02;
/// Grace notes sound this long before their note and before each other.
constexpr double kGraceLeadSeconds = 0.05;
constexpr int kNoHand = -1;

bool withinStretch(const Shape& shape, const Placement& added)
{
    int lowest = added.fret > 0 ? added.fret : std::numeric_limits<int>::max();
    int highest = added.fret;
    for (const auto& placement : shape)
        if (placement.string != Placement::kUnplayable && placement.fret > 0)
        {
            lowest = std::min(lowest, placement.fret);
            highest = std::max(highest, placement.fret);
        }
    return highest - lowest <= kMaxStretch;
}

/// The places of each pitch from the lowest string up, then unplayable.
std::vector<std::vector<Placement>> placesOf(std::span<const int> pitches, const Fretboard& fretboard)
{
    std::vector<std::vector<Placement>> places(pitches.size());
    for (std::size_t index = 0; index < pitches.size(); ++index)
    {
        for (std::size_t string = 0; string < fretboard.strings.size(); ++string)
        {
            const int fret = pitches[index] - fretboard.strings[string];
            if (fret >= 0 && fret <= fretboard.frets)
                places[index].push_back({.string = static_cast<int>(string), .fret = fret});
        }
        places[index].emplace_back();
    }
    return places;
}

/// Backtracking without recursion over the places of the pitches, as the shape grows from the first pitch.
class ShapeSearch
{
public:
    ShapeSearch(std::span<const int> pitches, const Fretboard& fretboard)
        : places_(placesOf(pitches, fretboard)), choice_(pitches.size()), used_(fretboard.strings.size())
    {
    }

    [[nodiscard]] std::vector<Shape> run(std::size_t allowedUnplayable)
    {
        allowed_ = allowedUnplayable;
        std::vector<Shape> found;
        std::size_t depth = 0;
        while (true)
        {
            if (depth == places_.size())
                found.push_back(shape_);
            else if (choice_[depth] < places_[depth].size())
            {
                if (tryPlace(places_[depth][choice_[depth]]))
                    ++depth;
                else
                    ++choice_[depth];
                continue;
            }
            else
                choice_[depth] = 0;
            if (depth == 0)
                return found;
            --depth;
            removeLast();
            ++choice_[depth];
        }
    }

private:
    bool tryPlace(const Placement& placement)
    {
        if (placement.string == Placement::kUnplayable)
        {
            if (unplayable_ == allowed_)
                return false;
            ++unplayable_;
        }
        else if (used_[static_cast<std::size_t>(placement.string)] || !withinStretch(shape_, placement))
            return false;
        else
            used_[static_cast<std::size_t>(placement.string)] = true;
        shape_.push_back(placement);
        return true;
    }

    void removeLast()
    {
        if (shape_.back().string == Placement::kUnplayable)
            --unplayable_;
        else
            used_[static_cast<std::size_t>(shape_.back().string)] = false;
        shape_.pop_back();
    }

    std::vector<std::vector<Placement>> places_;
    std::vector<std::size_t> choice_;
    std::vector<bool> used_;
    Shape shape_;
    std::size_t unplayable_ = 0;
    std::size_t allowed_ = 0;
};

/// The fret of the index finger: the lowest fretted note, none if the shape uses open strings only.
int handOf(const Shape& shape)
{
    int hand = kNoHand;
    for (const auto& placement : shape)
        if (placement.string != Placement::kUnplayable && placement.fret > 0 &&
            (hand == kNoHand || placement.fret < hand))
            hand = placement.fret;
    return hand;
}

double shapeCost(const Shape& shape)
{
    double cost = 0.0;
    int lowest = std::numeric_limits<int>::max();
    int highest = 0;
    for (const auto& placement : shape)
    {
        if (placement.string == Placement::kUnplayable)
            cost += kUnplayableCost;
        else if (placement.fret > 0)
        {
            lowest = std::min(lowest, placement.fret);
            highest = std::max(highest, placement.fret);
        }
    }
    if (highest > 0)
        cost += (kStretchCost * (highest - lowest)) + (kHeightCost * lowest);
    return cost;
}

/// A shape of a chord with the hand position it leaves.
struct State
{
    std::size_t shape = 0;
    int hand = kNoHand;
    double cost = 0.0;
    std::size_t previous = 0;
};

double shiftCost(int from, int to, double gapSeconds)
{
    if (from == kNoHand)
        return 0.0;
    return kShiftCost * std::abs(to - from) * (gapSeconds >= kPauseSeconds ? kPauseFactor : 1.0);
}

/// The cheapest way to reach a shape with a known hand position from the states of the previous chord.
State reachFretted(std::size_t shape, int hand, double cost, const std::vector<State>& before, double gap)
{
    State best{.shape = shape, .hand = hand, .cost = std::numeric_limits<double>::infinity(), .previous = 0};
    for (std::size_t index = 0; index < before.size(); ++index)
    {
        const double total = before[index].cost + shiftCost(before[index].hand, hand, gap) + cost;
        if (total < best.cost)
        {
            best.cost = total;
            best.previous = index;
        }
    }
    return best;
}

/// An open shape keeps each hand position of the previous chord: one state per position.
void reachOpen(std::size_t shape, double cost, const std::vector<State>& before, std::vector<State>& states)
{
    const auto first = states.size();
    for (std::size_t index = 0; index < before.size(); ++index)
    {
        const double total = before[index].cost + cost;
        const auto same = std::ranges::find(states.begin() + static_cast<std::ptrdiff_t>(first),
                                            states.end(),
                                            before[index].hand,
                                            &State::hand);
        if (same == states.end())
            states.push_back({.shape = shape, .hand = before[index].hand, .cost = total, .previous = index});
        else if (total < same->cost)
        {
            same->cost = total;
            same->previous = index;
        }
    }
}

std::vector<State>
nextStates(const std::vector<Shape>& shapes, const std::vector<State>& before, double gapSeconds)
{
    std::vector<State> states;
    for (std::size_t shape = 0; shape < shapes.size(); ++shape)
    {
        const int hand = handOf(shapes[shape]);
        const double cost = shapeCost(shapes[shape]);
        if (hand == kNoHand)
            reachOpen(shape, cost, before, states);
        else
            states.push_back(reachFretted(shape, hand, cost, before, gapSeconds));
    }
    return states;
}

/// True if the shape plays every pitch with a fixed string on that string.
bool fits(const Shape& shape, const Strings& strings)
{
    const auto count = std::min(shape.size(), strings.size());
    for (std::size_t index = 0; index < count; ++index)
        if (strings[index] != Placement::kUnplayable && shape[index].string != strings[index])
            return false;
    return true;
}

/// The shapes that fit the fixed strings; all of them if none does.
std::vector<Shape> shapesOn(std::vector<Shape> shapes, const Strings& strings)
{
    std::vector<Shape> fitting;
    for (const auto& shape : shapes)
        if (fits(shape, strings))
            fitting.push_back(shape);
    if (fitting.empty())
        return shapes;
    return fitting;
}

} // namespace

Fretboard fretboardOf(const model::InstrumentProfile& profile)
{
    return {.strings = profile.strings, .frets = profile.frets};
}

std::vector<Shape> playableShapes(std::span<const int> pitches, const Fretboard& fretboard)
{
    ShapeSearch search{pitches, fretboard};
    for (std::size_t allowed = 0;; ++allowed)
    {
        auto shapes = search.run(allowed);
        if (!shapes.empty())
            return shapes;
    }
}

std::vector<Shape> fingering(const std::vector<Chord>& chords, const Fretboard& fretboard)
{
    std::vector<std::vector<Shape>> shapes;
    shapes.reserve(chords.size());
    std::vector<std::vector<State>> states;
    states.reserve(chords.size());
    std::vector<State> before{State{}};
    double previousSeconds = 0.0;
    for (const auto& chord : chords)
    {
        shapes.push_back(shapesOn(playableShapes(chord.pitches, fretboard), chord.strings));
        states.push_back(nextStates(shapes.back(), before, chord.seconds - previousSeconds));
        before = states.back();
        previousSeconds = chord.seconds;
    }
    std::vector<Shape> result(chords.size());
    if (chords.empty())
        return result;
    auto state = static_cast<std::size_t>(std::ranges::min_element(states.back(), {}, &State::cost) -
                                          states.back().begin());
    for (std::size_t chord = chords.size(); chord-- > 0;)
    {
        result[chord] = shapes[chord][states[chord][state].shape];
        state = states[chord][state].previous;
    }
    return result;
}

std::vector<EventTab> tabulate(const std::vector<rhythm::QuantizedEvent>& events,
                               const Fretboard& fretboard,
                               const std::vector<Strings>& strings)
{
    std::vector<Chord> chords;
    for (std::size_t index = 0; index < events.size(); ++index)
    {
        const auto& event = events[index];
        const auto count = event.graces.size();
        for (std::size_t grace = 0; grace < count; ++grace)
            chords.push_back(
                {.seconds = event.seconds - (kGraceLeadSeconds * static_cast<double>(count - grace)),
                 .pitches = event.graces[grace],
                 .strings = {}});
        chords.push_back({.seconds = event.seconds, .pitches = event.pitches, .strings = {}});
        if (index < strings.size())
            chords.back().strings = strings[index];
    }
    auto shapes = fingering(chords, fretboard);
    std::vector<EventTab> result;
    result.reserve(events.size());
    std::size_t next = 0;
    for (const auto& event : events)
    {
        EventTab tab;
        for (std::size_t grace = 0; grace < event.graces.size(); ++grace)
            tab.graces.push_back(std::move(shapes[next++]));
        tab.notes = std::move(shapes[next++]);
        result.push_back(std::move(tab));
    }
    return result;
}

} // namespace snotes::tab
