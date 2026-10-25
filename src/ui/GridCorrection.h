#pragma once

#include "io/IFileSystem.h"

#include <juce_data_structures/juce_data_structures.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace snotes::ui
{

/// A meter the analysis suggests, as offered for choice (F-KO-01): "3/4 (20 %)".
struct MeterChoice
{
    std::string label;
    /// The chosen meter.
    bool ticked = false;

    bool operator==(const MeterChoice&) const = default;
};

struct GridEnvironment
{
    /// Folder of the open project; std::nullopt while it has none.
    std::function<std::optional<std::filesystem::path>()> projectFolder;
    std::function<void(const std::string& title, const std::string& message)> showMessage;
};

/// Presenter for the correction of the beat grid (F-KO-01, F-KO-02): every change is stored as the user's
/// grid, which a later note detection keeps, and the events and the score of all voices are quantized on it
/// again without detecting the notes again (ui::updateRhythm).
class GridCorrection
{
public:
    /// Beats closer to each other than this are refused.
    static constexpr double kShortestBeatSeconds = 0.05;

    GridCorrection(juce::ValueTree project, io::IFileSystem& fileSystem, GridEnvironment environment);

    /// The readings of the analysis, most likely first; the one with the chosen number of beats is ticked.
    [[nodiscard]] std::vector<MeterChoice> meterChoices() const;
    /// Takes the number of beats and the first downbeat of the reading.
    void chooseMeter(std::size_t reading);

    /// The beat closest to the time if it is no further away than tolerance.
    [[nodiscard]] std::optional<std::size_t> beatNear(double seconds, double toleranceSeconds) const;
    /// The beat stays between its neighbours.
    void moveBeat(std::size_t beat, double seconds);
    /// Nothing happens without a grid or too close to a beat. The downbeats before the new beat stay where
    /// they are.
    void insertBeat(double seconds);
    /// A grid keeps at least two beats. The downbeats before the beat stay where they are.
    void deleteBeat(std::size_t beat);
    /// The beat becomes the first beat of a bar.
    void setDownbeat(std::size_t beat);
    /// Forgets the corrections and detects the beats and the meter again.
    void detectAgain();

private:
    void store(const std::vector<std::int64_t>& beats, int meter, int phase);
    void update();
    [[nodiscard]] std::vector<std::int64_t> beats() const;
    [[nodiscard]] std::int64_t sampleAt(double seconds) const;

    juce::ValueTree project_;
    io::IFileSystem& fileSystem_;
    GridEnvironment environment_;
};

} // namespace snotes::ui
