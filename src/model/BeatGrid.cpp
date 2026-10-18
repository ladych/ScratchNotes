#include "model/BeatGrid.h"

#include "model/ModelIds.h"

#include <sstream>
#include <utility>

namespace snotes::model
{

BeatGrid::BeatGrid(juce::ValueTree tree) : TreeNode(std::move(tree), nullptr) {}

std::vector<std::int64_t> BeatGrid::beats() const
{
    std::istringstream text{getString(ids::beats)};
    std::vector<std::int64_t> beats;
    for (std::int64_t beat = 0; text >> beat;)
        beats.push_back(beat);
    return beats;
}

std::string BeatGrid::source() const
{
    return getString(ids::source);
}

std::vector<MeterReading> BeatGrid::meterReadings() const
{
    std::vector<MeterReading> readings;
    for (const auto& child : tree())
        readings.push_back({.numerator = child[ids::numerator],
                            .confidence = child[ids::confidence],
                            .phase = child[ids::phase]});
    return readings;
}

int BeatGrid::chosenMeter() const
{
    return getInt(ids::chosenMeter, 0);
}

int BeatGrid::downbeatPhase() const
{
    return getInt(ids::downbeatPhase, 0);
}

void BeatGrid::setAnalysis(const std::vector<std::int64_t>& beats, const std::vector<MeterReading>& readings)
{
    clear();
    std::ostringstream text;
    for (const auto beat : beats)
        text << (text.tellp() == 0 ? "" : " ") << beat;
    setString(ids::beats, text.str());
    setString(ids::source, "auto");
    for (const auto& reading : readings)
    {
        juce::ValueTree child{ids::meterReading};
        child.setProperty(ids::numerator, reading.numerator, nullptr);
        child.setProperty(ids::confidence, reading.confidence, nullptr);
        child.setProperty(ids::phase, reading.phase, nullptr);
        mutableTree().appendChild(child, nullptr);
    }
    if (!readings.empty())
    {
        set(ids::chosenMeter, readings.front().numerator);
        set(ids::downbeatPhase, readings.front().phase);
    }
}

void BeatGrid::clear()
{
    mutableTree().removeAllProperties(nullptr);
    mutableTree().removeAllChildren(nullptr);
}

} // namespace snotes::model
