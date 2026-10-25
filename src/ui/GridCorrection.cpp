#include "ui/GridCorrection.h"

#include "i18n/Translate.h"
#include "model/Project.h"
#include "ui/RhythmUpdate.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

namespace snotes::ui
{

GridCorrection::GridCorrection(juce::ValueTree project,
                               io::IFileSystem& fileSystem,
                               GridEnvironment environment)
    : project_(std::move(project)), fileSystem_(fileSystem), environment_(std::move(environment))
{
}

std::vector<MeterChoice> GridCorrection::meterChoices() const
{
    const auto grid = model::Project{project_, nullptr}.beatGrid();
    std::vector<MeterChoice> choices;
    for (const auto& reading : grid.meterReadings())
        choices.push_back({.label = std::to_string(reading.numerator) + "/4 (" +
                                    std::to_string(std::lround(reading.confidence * 100.0)) + " %)",
                           .ticked = reading.numerator == grid.chosenMeter()});
    return choices;
}

void GridCorrection::chooseMeter(std::size_t reading)
{
    const auto readings = model::Project{project_, nullptr}.beatGrid().meterReadings();
    if (reading >= readings.size())
        return;
    store(beats(), readings[reading].numerator, readings[reading].phase);
}

std::optional<std::size_t> GridCorrection::beatNear(double seconds, double toleranceSeconds) const
{
    const auto all = beats();
    const auto sampleRate = model::Project{project_, nullptr}.sampleRate();
    std::optional<std::size_t> nearest;
    double distance = toleranceSeconds;
    for (std::size_t beat = 0; beat < all.size(); ++beat)
    {
        const auto away = std::abs(static_cast<double>(all[beat]) / sampleRate - seconds);
        if (away > distance)
            continue;
        nearest = beat;
        distance = away;
    }
    return nearest;
}

void GridCorrection::moveBeat(std::size_t beat, double seconds)
{
    auto all = beats();
    if (beat >= all.size())
        return;
    const auto gap = sampleAt(kShortestBeatSeconds);
    auto sample = sampleAt(seconds);
    if (beat > 0)
        sample = std::max(sample, all[beat - 1] + gap);
    if (beat + 1 < all.size())
        sample = std::min(sample, all[beat + 1] - gap);
    all[beat] = sample;
    const auto grid = model::Project{project_, nullptr}.beatGrid();
    store(all, grid.chosenMeter(), grid.downbeatPhase());
}

void GridCorrection::insertBeat(double seconds)
{
    auto all = beats();
    if (all.size() < 2)
        return;
    const auto sample = sampleAt(seconds);
    const auto gap = sampleAt(kShortestBeatSeconds);
    const auto next = std::ranges::lower_bound(all, sample);
    if ((next != all.end() && *next - sample < gap) ||
        (next != all.begin() && sample - *std::prev(next) < gap))
        return;
    all.insert(next, sample);
    const auto grid = model::Project{project_, nullptr}.beatGrid();
    store(all, grid.chosenMeter(), grid.downbeatPhase());
}

void GridCorrection::deleteBeat(std::size_t beat)
{
    auto all = beats();
    if (beat >= all.size() || all.size() <= 2)
        return;
    all.erase(all.begin() + static_cast<std::ptrdiff_t>(beat));
    const auto grid = model::Project{project_, nullptr}.beatGrid();
    store(all, grid.chosenMeter(), grid.downbeatPhase());
}

void GridCorrection::setDownbeat(std::size_t beat)
{
    const auto all = beats();
    const auto grid = model::Project{project_, nullptr}.beatGrid();
    const int meter = grid.chosenMeter();
    if (beat >= all.size() || meter <= 0)
        return;
    store(all, meter, static_cast<int>(beat) % meter);
}

void GridCorrection::detectAgain()
{
    model::Project{project_, nullptr}.beatGrid().clear();
    update();
}

void GridCorrection::store(const std::vector<std::int64_t>& beats, int meter, int phase)
{
    model::Project{project_, nullptr}.beatGrid().setEdited(beats, meter, phase);
    update();
}

void GridCorrection::update()
{
    const auto folder = environment_.projectFolder();
    if (!folder)
        return;
    if (updateRhythm(project_, fileSystem_, *folder) == RhythmUpdate::writeFailed)
        environment_.showMessage(i18n::tr("Beat grid"),
                                 i18n::tr("Writing the rhythm failed; is the disk full?"));
}

std::vector<std::int64_t> GridCorrection::beats() const
{
    return model::Project{project_, nullptr}.beatGrid().beats();
}

std::int64_t GridCorrection::sampleAt(double seconds) const
{
    return std::llround(seconds * model::Project{project_, nullptr}.sampleRate());
}

} // namespace snotes::ui
