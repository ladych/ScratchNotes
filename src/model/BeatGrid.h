#pragma once

#include "model/TreeNode.h"

#include <cstdint>
#include <string>
#include <vector>

namespace snotes::model
{

/// A meter the analysis suggests (F-TX-06).
struct MeterReading
{
    /// Beats per bar, 3 to 7.
    int numerator = 4;
    /// 0..1, all readings add up to 1.
    double confidence = 0.0;
    /// Index of the first beat that starts a bar.
    int phase = 0;

    bool operator==(const MeterReading&) const = default;
};

/// The beat grid all voices share (F-TX-05), with the meter suggestions: BEAT_GRID with the beats as sample
/// positions of the song, separated by spaces, and one METER_READING per suggestion, most likely first.
/// Not undoable: it is the result of an analysis, like the notes.
class BeatGrid : public TreeNode
{
public:
    explicit BeatGrid(juce::ValueTree tree);

    [[nodiscard]] std::vector<std::int64_t> beats() const;
    /// "auto" after the analysis; "edited" is for the correction (F-KO-02).
    [[nodiscard]] std::string source() const;
    [[nodiscard]] std::vector<MeterReading> meterReadings() const;
    /// Beats per bar of the chosen meter, 0 without a grid.
    [[nodiscard]] int chosenMeter() const;
    [[nodiscard]] int downbeatPhase() const;

    /// Stores an analysis; the most likely meter becomes the chosen one.
    void setAnalysis(const std::vector<std::int64_t>& beats, const std::vector<MeterReading>& readings);
    void clear();
};

} // namespace snotes::model
